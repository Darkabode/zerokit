#ifndef __ZGUI_STOPWATCH_H_
#define __ZGUI_STOPWATCH_H_

namespace zgui {

class Stopwatch
{
public:
	Stopwatch();
	~Stopwatch();

	void start();
	void stop();
	void reset();
	void restart();

	int64_t elapsed() const;
	int elapsedSeconds() const;

private:
	Stopwatch(const Stopwatch&);
	Stopwatch& operator = (const Stopwatch&);

	int64_t update() const;

	int64_t _start;
	int64_t _elapsed;
	bool _running;
};

}

#endif // __ZGUI_STOPWATCH_H_
