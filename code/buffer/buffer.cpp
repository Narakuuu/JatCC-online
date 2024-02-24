#include "buffer.h"

//初始化缓冲区，指定初始大小为 initBuffSize。
Buffer::Buffer(int initBuffSize):buffer_(initBuffSize),readPos_(0),writePos_(0){}


//返回可读的字节数
size_t Buffer::ReadableBytes() const{
    return writePos_ - readPos_;
}

//返回可写的字节数
size_t Buffer::WritableBytes() const{
    return buffer_.size() - writePos_;
}

//返回在读位置之前的字节数
size_t Buffer::PrependableBytes() const{
    return readPos_;
}

//返回readPos_当前位置的指针，获取当前可读取数据的起始位置的指针
const char* Buffer:: Peek() const {
    return BeginPtr_() + readPos_;
}

//移动读指针(readPos_) len 的距离，表示已读取了 len 字节的数据
void Buffer::Retrieve(size_t len){
    assert( len <= ReadableBytes());
    readPos_ += len;
}

//移动读指针到指定的位置 end
void Buffer::RetrieveUntil(const char* end){
    assert(Peek() <= end);
    Retrieve(end - Peek());
}

//清空缓冲区，将读写指针归零。
void Buffer::RetrieveAll(){
    bzero(&buffer_[0],buffer_.size());
    //buffer_.clear();

    readPos_ = 0;
    writePos_ = 0;
}


//获得可读取的所有数据，并清空缓冲区返回字符串形式
std::string Buffer::RetrieveAllToStr(){
    //Peek() 返回的指针和 ReadableBytes() 返回的可读字节数构造字符串。
    //Peek() 当前可读数据的起始位置的指针
    std::string str(Peek(), ReadableBytes());
    //清空缓存区
    RetrieveAll();
    return str;
}
 
//返回一个常量指针，指向缓冲区当前可写数据的起始位置
const char* Buffer::BeginWriteConst() const{
    return BeginPtr_()+ writePos_;
}


//返回一个可写指针，指向缓冲区当前可写数据的起始位置
char* Buffer::BeginWrite(){
    return BeginPtr_() + writePos_;
}

//移动写指针 writePos_ 的位置，表示已写入了 len 字节长度的数据
void Buffer::HasWritten(size_t len){
    writePos_ += len;
}


void Buffer::Append(const std::string& str){
    Append(str.data(),str.length());
}

void Buffer:: Append(const void* data,size_t len){
    assert(data);
    Append(static_cast<const char*>(data), len);
}

void Buffer::Append(const char* str, size_t len){
    assert(str);
    EnsureWriteable(len);
    std::copy(str,str+len,BeginWrite());
    HasWritten(len);
}

void Buffer::Append(const Buffer& buffer){
    Append(buffer.Peek(),buffer.ReadableBytes());
}


void Buffer::EnsureWriteable(size_t len){
    if(WritableBytes() < len){
        MakeSpace_(len);
    }
    assert(WritableBytes() >= len);
}

/*将文件描述符fd上的数据读取（写）到缓冲区buffer_中*/
ssize_t Buffer::ReadFd(int fd, int* saveErrno){
    char buff[65535];//临时缓冲区，用于存储 readv 函数未能完全读取的数据
    struct iovec iov[2];// iov结构用于分散读 
    const size_t writeable = WritableBytes(); //获取当前可写的字节数，readv 函数要确保写入的数据不超过缓冲区的可写容量。
    /* 分散读，保证数据全部读完*/
    iov[0].iov_base = BeginPtr_() + writePos_; //iov[0] 指向缓冲区中可写数据的起始位置
    iov[0].iov_len = writeable; // iov[0] 的长度为当前可写字节数
    iov[1].iov_base = buff; // iov[1] 指向临时缓冲区
    iov[1].iov_len = sizeof(buff); // iov[1] 的长度为临时缓冲区的大小

    /* 从文件描述fd中读取数据，，将数据写入到指定的缓冲区数组 iov 中*/
    const ssize_t len = readv(fd,iov,2); // 使用 readv 函数进行分散读取
    if( len < 0 ){
        *saveErrno = errno; // 读取出错，保存错误码
    }
    /* 读取的数据不超过当前可写字节数，直接更新写指针*/
    else if(static_cast<size_t>(len) <= writeable){
        writePos_ += len;
    }
    /* 读取的数据超过当前可写字节数，读取到的数据超出了当前缓冲区的可写容量*/
    else{   
        writePos_ = buffer_.size(); //写指针更新到缓冲区末尾
        Append(buff,len-writeable); // 将多出的数据追加到缓冲区
    }
    return len; //返回读取的总字节数
}

/*将缓冲区内的数据写入到文件描述符fd中*/
ssize_t Buffer::WriteFd(int fd, int* saveErrno){
    //获取当前缓冲区可读数据的字节数
    size_t readsize = ReadableBytes();  
    //write 将可读数据写入到文件描述符中，len 表示成功写入的字节数。write 函数将成功写入的数据从缓冲区中消耗掉。
    size_t len = write(fd, Peek(),readsize);
    if( len < 0 ){
        *saveErrno = errno;
        return len;
    } 
    //更新读指针，表示已成功写入了 len 字节的数据
    readPos_ += len;
    return len;
}

char* Buffer::BeginPtr_(){
    //强制转换，对迭代器解引用后取地址，得到指向第一个元素的指针
    return &*buffer_.begin();
}

const char* Buffer::BeginPtr_() const {
    return &*buffer_.begin();
}


/*确保缓冲区有足够空间容纳新数据*/
void Buffer::MakeSpace_(size_t len){
    /*
    WritableBytes()：表示当前缓冲区中从写指针 (writePos_) 到缓冲区末尾可用的字节数。这是可以直接写入新数据的空间。
    PrependableBytes(): 表示当前缓冲区中从缓冲区起始位置到读指针 (readPos_) 的字节数。这是已读取但未被消耗的字节
    WritableBytes() + PrependableBytes(): 表示当前缓冲区中总共可以写入新数据的字节数。即可写空间和前置字节的总和
    */
   //如果空间不够，则resize调整缓冲区的大小，确保可以容纳 len 长度的新数据
    if(WritableBytes() + PrependableBytes() < len){
        buffer_.resize(writePos_+len+1);
    }
    //如果空间足够
    else{
        //获取当前可读数据的字节数
        size_t readable = ReadableBytes();
        //可读数据移动到缓冲区的起始位置。这样做的目的是为了释放已经读取的数据的空间，给新数据腾出位置
        std::copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_());
        //将读指针移动到缓冲区的起始位置
        readPos_ = 0;
        //更新写指针，表示新的写入位置。
        writePos_ = readPos_ + readable;
        assert(readable == ReadableBytes());
    }
}