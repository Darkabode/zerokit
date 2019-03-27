namespace zgui {

Stopwatch::Stopwatch() :
_elapsed(0),
_running(false)
{
}


Stopwatch::~Stopwatch()
{
}


int64_t Stopwatch::elapsed() const
{
	if (_running) {
		int64_t current = update();
		return _elapsed + (current - _start);
	}
	else {
		return _elapsed;
	}
}


void Stopwatch::reset()
{
	_elapsed = 0;
	_running = false;
}


void Stopwatch::restart()
{
	_elapsed = 0;
	_start = update();
	_running = true;
}

void Stopwatch::start()
{
	if (!_running) {
		_start = update();
		_running = true;
	}
}


void Stopwatch::stop()
{
	if (_running) {
		int64_t current = update();
		_elapsed += current - _start;
		_running = false;
	}
}


int Stopwatch::elapsedSeconds() const
{
	return int(elapsed() / 1000000);
}

int64_t Stopwatch::update() const
{
	LARGE_INTEGER perfCounter;
	LARGE_INTEGER perfFreq;

	if (fn_QueryPerformanceCounter(&perfCounter) && fn_QueryPerformanceFrequency(&perfFreq)) {
		return perfCounter.QuadPart * 1000000 / perfFreq.QuadPart;
	}

	return 0;
}

}