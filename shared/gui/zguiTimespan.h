#ifndef __ZGUI_TIMESPAN_H_
#define __ZGUI_TIMESPAN_H_

namespace zgui {

class Timespan
{
public:
	Timespan();
	/// Creates a zero Timespan.

	Timespan(int64_t microseconds);
	/// Creates a Timespan.

	Timespan(long seconds, long microseconds);
	/// Creates a Timespan. Useful for creating
	/// a Timespan from a struct timeval.

	Timespan(int days, int hours, int minutes, int seconds, int microSeconds);
	/// Creates a Timespan.

	Timespan(const Timespan& timespan);
	/// Creates a Timespan from another one.

	~Timespan();
	/// Destroys the Timespan.

	Timespan& operator = (const Timespan& timespan);
	/// Assignment operator.

	Timespan& operator = (int64_t microseconds);
	/// Assignment operator.

	Timespan& assign(int days, int hours, int minutes, int seconds, int microSeconds);
	/// Assigns a new span.

	Timespan& assign(long seconds, long microseconds);
	/// Assigns a new span. Useful for assigning
	/// from a struct timeval.

	bool operator == (const Timespan& ts) const;
	bool operator != (const Timespan& ts) const;
	bool operator >  (const Timespan& ts) const;
	bool operator >= (const Timespan& ts) const;
	bool operator <  (const Timespan& ts) const;
	bool operator <= (const Timespan& ts) const;

	bool operator == (int64_t microSeconds) const;
	bool operator != (int64_t microSeconds) const;
	bool operator >  (int64_t microSeconds) const;
	bool operator >= (int64_t microSeconds) const;
	bool operator <  (int64_t microSeconds) const;
	bool operator <= (int64_t microSeconds) const;

	Timespan operator + (const Timespan& d) const;
	Timespan operator - (const Timespan& d) const;
	Timespan& operator += (const Timespan& d);
	Timespan& operator -= (const Timespan& d);

	Timespan operator + (int64_t microSeconds) const;
	Timespan operator - (int64_t microSeconds) const;
	Timespan& operator += (int64_t microSeconds);
	Timespan& operator -= (int64_t microSeconds);

	int days() const;
	/// Returns the number of days.

	int hours() const;
	/// Returns the number of hours (0 to 23).

	int totalHours() const;
	/// Returns the total number of hours.

	int minutes() const;
	/// Returns the number of minutes (0 to 59).

	int totalMinutes() const;
	/// Returns the total number of minutes.

	int seconds() const;
	/// Returns the number of seconds (0 to 59).

	int totalSeconds() const;
	/// Returns the total number of seconds.

	int milliseconds() const;
	/// Returns the number of milliseconds (0 to 999).

	int64_t totalMilliseconds() const;
	/// Returns the total number of milliseconds.

	int microseconds() const;
	/// Returns the fractions of a millisecond
	/// in microseconds (0 to 999).

	int useconds() const;
	/// Returns the fractions of a second
	/// in microseconds (0 to 999999).

	int64_t totalMicroseconds() const;
	/// Returns the total number of microseconds.

	static const int64_t MILLISECONDS; /// The number of microseconds in a millisecond.
	static const int64_t SECONDS;      /// The number of microseconds in a second.
	static const int64_t MINUTES;      /// The number of microseconds in a minute.
	static const int64_t HOURS;        /// The number of microseconds in a hour.
	static const int64_t DAYS;         /// The number of microseconds in a day.

private:
	int64_t _span;
};

}

#endif // __ZGUI_TIMESPAN_H_
