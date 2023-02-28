#ifndef _SINGLETON_H_A5EBD2FE_F21F_40CD_93AC_D2FA7AD211F0
#define _SINGLETON_H_A5EBD2FE_F21F_40CD_93AC_D2FA7AD211F0

// https://stackoverflow.com/questions/1008019/c-singleton-design-pattern
// https://codereview.stackexchange.com/questions/173929/modern-c-singleton-template

template<typename T>
class Singleton {
public:   
    static T& instance() {
        //static T instance{token{}};
        static T instance;
        return instance;
    }

    Singleton(const Singleton&) = delete;
    Singleton& operator= (const Singleton) = delete;

protected:
    //struct token {};
    Singleton() {}
};


#endif // _SINGLETON_H_A5EBD2FE_F21F_40CD_93AC_D2FA7AD211F0
