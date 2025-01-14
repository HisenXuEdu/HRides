#ifndef SERIALIZER_HPP
#define SERIALIZER_HPP

#include <vector>
using namespace std;


class StreamBuffer : public std::vector<char>
{
public:
    StreamBuffer():m_curpos(0){}

    StreamBuffer(const char* in, size_t len)
    {
        m_curpos = 0;
        insert(begin(), in, in+len);
    }

    const char* current(){ 
        return&(*this)[m_curpos]; 
    } // 获取当前位置的数据

    void offset(int offset){ m_curpos += offset; } //移动当前位置

    void input(const char* in, size_t len) //输入字符数组
    {
        insert(end(), in, in+len);
    }

    bool is_eof(){ return m_curpos >= size(); } //检查是否已经到达末尾

    void reset(){m_curpos=0;}

    const char* data(){ return &(*this)[0]; } //获取缓冲区的数据，只是vector对[]的运算符重载

private:
    unsigned int m_curpos; //当前字节流的位置
};


class Serializer
{
public:
    enum ByteOrder   //大小端序
    {
        BigEndian = 0,
        LittleEndian = 1
    };

    Serializer(){
        m_byteorder = LittleEndian;
    }
    Serializer(StreamBuffer& dev, int byteorder = LittleEndian){
        m_byteorder = byteorder;
        m_iodevice = dev;
    }
    ~Serializer(){};

    void reset(){ 
        m_iodevice.reset();
    }

    void write_raw_data(char* in, int len){\
		m_iodevice.input(in, len);
		m_iodevice.offset(len);
	}

    void byte_orser(char* in, int len){
		if (m_byteorder == BigEndian){
			reverse(in, in+len); //大端的化直接反转 ，网络传输是大端序
		}
	}

    void clear(){
		m_iodevice.clear();
		reset();
	}

    int size(){
		return m_iodevice.size();
	}

    const char* data(){
        return m_iodevice.data();
    }


    template<typename T>
	void output_type(T& t);

    template<typename T>
	void input_type(T t);

    template<typename T>
    Serializer &operator >>(T& i){
        output_type(i); 
        return *this;
    }

    template<typename T>
	Serializer &operator << (T i){
		input_type(i);
		return *this;
	}    

private:
    int m_byteorder; //字节序
    StreamBuffer m_iodevice; //字节流缓冲区
};

template<typename T>
void Serializer::output_type(T& t){
	int len = sizeof(T);         //相当于传递的时候是按照顺序的，所以在读出的时候就按照T的大小直接copy进去对应的位置,这就是value_t<R>类型
	char* d = new char[len];
	if (!m_iodevice.is_eof()){
		memcpy(d, m_iodevice.current(), len);   //字节流头部复制到最后，因为rpc中调用了Serializer的reset，所以m_curpos在头部
		m_iodevice.offset(len);
		byte_orser(d, len);
		t = *reinterpret_cast<T*>(&d[0]);
	}
	delete [] d;
}

template<>
inline void Serializer::output_type(std::string& in){
    int marklen = sizeof(uint16_t); //读取长度

    char* d = new char[marklen];
    memcpy(d, m_iodevice.current(), marklen); //将字节流的数据的两个字节拷贝到d中
    int len = *reinterpret_cast<uint16_t*>(&d[0]);  //取出长度
    byte_orser(d, marklen);
    m_iodevice.offset(marklen); //将字节流的位置向后移动两个字节，字节流前两个字节放的是这个请求的长度
    delete d;
    if(len == 0) return;

    in.insert(in.begin(), m_iodevice.current(), m_iodevice.current() + len); //输入到in中
	m_iodevice.offset(len);
}

template<>
inline void Serializer::input_type(std::string in)
{
    //先将字符串的长度输入到字节流中
	uint16_t len = in.size();    //先计算string的长度，放入m_iodevice.input字节流的最前面
	char* p = reinterpret_cast<char*>(&len);
	byte_orser(p, sizeof(uint16_t));
	m_iodevice.input(p, sizeof(uint16_t)); 
	if (len == 0) return;

    //再将字符串的数据输入到字节流中
	char* d = new char[len];
	memcpy(d, in.c_str(), len); //将字符串的数据拷贝到d中
	m_iodevice.input(d, len);
	delete [] d;
}


#endif