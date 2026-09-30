#ifndef _DEVICE_HPP
#define _DEVICE_HPP

class Device {
    public:
    virtual ~Device() = default;

    virtual bool init(void) = 0;
    virtual void deinit(void) = 0;
};

#endif