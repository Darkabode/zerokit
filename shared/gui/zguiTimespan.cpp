namespace zgui {

const int64_t Timespan::MILLISECONDS = 1000;
const int64_t Timespan::SECONDS = 1000 * Timespan::MILLISECONDS;
const int64_t Timespan::MINUTES = 60 * Timespan::SECONDS;
const int64_t Timespan::HOURS = 60 * Timespan::MINUTES;
const int64_t Timespan::DAYS = 24 * Timespan::HOURS;


Timespan::Timespan() :
_span(0)
{
}


Timespan::Timespan(int64_t microSeconds) :
_span(microSeconds)
{
}


Timespan::Timespan(long seconds, long microSeconds) :
_span(int64_t(seconds)*SECONDS + microSeconds)
{
}


Timespan::Timespan(int days, int hours, int minutes, int seconds, int microSeconds) :
_span(int64_t(microSeconds) + int64_t(seconds)*SECONDS + int64_t(minutes)*MINUTES + int64_t(hours)*HOURS + int64_t(days)*DAYS)
{
}


Timespan::Timespan(const Timespan& timespan) :
_span(timespan._span)
{
}


Timespan::~Timespan()
{
}


Timespan& Timespan::operator = (const Timespan& timespan)
{
	_span = timespan._span;
	return *this;
}


Timespan& Timespan::operator = (int64_t microSeconds)
{
	_span = microSeconds;
	return *this;
}


Timespan& Timespan::assign(int days, int hours, int minutes, int seconds, int microSeconds)
{
	_span = int64_t(microSeconds) + int64_t(seconds)*SECONDS + int64_t(minutes)*MINUTES + int64_t(hours)*HOURS + int64_t(days)*DAYS;
	return *this;
}


Timespan& Timespan::assign(long seconds, long microSeconds)
{
	_span = int64_t(seconds)*SECONDS + int64_t(microSeconds);
	return *this;
}

Timespan Timespan::operator + (const Timespan& d) const
{
	return Timespan(_span + d._span);
}


Timespan Timespan::operator - (const Timespan& d) const
{
	return Timespan(_span - d._span);
}


Timespan& Timespan::operator += (const Timespan& d)
{
	_span += d._span;
	return *this;
}


Timespan& Timespan::operator -= (const Timespan& d)
{
	_span -= d._span;
	return *this;
}


Timespan Timespan::operator + (int64_t microSeconds) const
{
	return Timespan(_span + microSeconds);
}


Timespan Timespan::operator - (int64_t microSeconds) const
{
	return Timespan(_span - microSeconds);
}


Timespan& Timespan::operator += (int64_t microSeconds)
{
	_span += microSeconds;
	return *this;
}


Timespan& Timespan::operator -= (int64_t microSeconds)
{
	_span -= microSeconds;
	return *this;
}

int Timespan::days() const
{
	return int(_span / DAYS);
}


int Timespan::hours() const
{
	return int((_span / HOURS) % 24);
}


int Timespan::totalHours() const
{
	return int(_span / HOURS);
}


int Timespan::minutes() const
{
	return int((_span / MINUTES) % 60);
}


int Timespan::totalMinutes() const
{
	return int(_span / MINUTES);
}


int Timespan::seconds() const
{
	return int((_span / SECONDS) % 60);
}


int Timespan::totalSeconds() const
{
	return int(_span / SECONDS);
}


int Timespan::milliseconds() const
{
	return int((_span / MILLISECONDS) % 1000);
}


int64_t Timespan::totalMilliseconds() const
{
	return _span / MILLISECONDS;
}


int Timespan::microseconds() const
{
	return int(_span % 1000);
}


int Timespan::useconds() const
{
	return int(_span % 1000000);
}


int64_t Timespan::totalMicroseconds() const
{
	return _span;
}


bool Timespan::operator == (const Timespan& ts) const
{
	return _span == ts._span;
}


bool Timespan::operator != (const Timespan& ts) const
{
	return _span != ts._span;
}


bool Timespan::operator >  (const Timespan& ts) const
{
	return _span > ts._span;
}


bool Timespan::operator >= (const Timespan& ts) const
{
	return _span >= ts._span;
}


bool Timespan::operator <  (const Timespan& ts) const
{
	return _span < ts._span;
}


bool Timespan::operator <= (const Timespan& ts) const
{
	return _span <= ts._span;
}


bool Timespan::operator == (int64_t microSeconds) const
{
	return _span == microSeconds;
}


bool Timespan::operator != (int64_t microSeconds) const
{
	return _span != microSeconds;
}


bool Timespan::operator >  (int64_t microSeconds) const
{
	return _span > microSeconds;
}


bool Timespan::operator >= (int64_t microSeconds) const
{
	return _span >= microSeconds;
}


bool Timespan::operator <  (int64_t microSeconds) const
{
	return _span < microSeconds;
}


bool Timespan::operator <= (int64_t microSeconds) const
{
	return _span <= microSeconds;
}

}