#include <memory>
#include <vector>
#include "global.h"
#include "RedisHelper.h"

class CommandParser{
protected:
    static std::shared_ptr<RedisHelper> redisHelper; //静态成员变量，所有解析器共享一个RedisHelper，实际的操作过程对象。
public:
    static std::shared_ptr<RedisHelper> getRedisHelper() { return redisHelper; }  //饿汉模式    
    virtual std::string parse(std::vector<std::string>& tokens) = 0;

};



// SetParser 
class SetParser : public CommandParser {
public:
    std::string parse(std::vector<std::string>& tokens) override;  //override让编译器检查是不是重载了父类虚函数
};