#ifndef __LOCKER_WINDOWSHIDER_H_
#define __LOCKER_WINDOWSHIDER_H_

class LockerWindowsHiderThread : public zgui::Thread
{
public:
    LockerWindowsHiderThread();
    ~LockerWindowsHiderThread();

    zgui_DeclareSingleton(LockerWindowsHiderThread)

private:
    void run();
};

#endif // __LOCKER_WINDOWSHIDER_H_
