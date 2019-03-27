namespace zgui {

StringPairArray::StringPairArray(const bool ignoreCase_) :
ignoreCase(ignoreCase_)
{
}

StringPairArray::StringPairArray(const StringPairArray& other) :
keys(other.keys),
values(other.values),
ignoreCase(other.ignoreCase)
{
}

StringPairArray::~StringPairArray()
{
}

StringPairArray& StringPairArray::operator=(const StringPairArray& other)
{
	keys = other.keys;
	values = other.values;
	return *this;
}

bool StringPairArray::operator==(const StringPairArray& other) const
{
	for (int i = keys.size(); --i >= 0;) {
		if (other[keys[i]] != values[i]) {
			return false;
		}
	}

	return true;
}

bool StringPairArray::operator!= (const StringPairArray& other) const
{
	return !operator==(other);
}

const String& StringPairArray::operator[](const String& key) const
{
	return values[keys.indexOf(key, ignoreCase)];
}

const String& StringPairArray::getValue(const String& key, const String& defaultReturnValue) const
{
	const int i = keys.indexOf(key, ignoreCase);

	if (i >= 0) {
		return values[i];
	}

	return defaultReturnValue;
}

void StringPairArray::set(const String& key, const String& value)
{
	const int i = keys.indexOf(key, ignoreCase);

	if (i >= 0) {
		values.set(i, value);
	}
	else {
		keys.add(key);
		values.add(value);
	}
}

void StringPairArray::addArray(const StringPairArray& other)
{
	for (int i = 0; i < other.size(); ++i) {
		set(other.keys[i], other.values[i]);
	}
}

void StringPairArray::clear()
{
	keys.clear();
	values.clear();
}

void StringPairArray::remove(const String& key)
{
	remove(keys.indexOf(key, ignoreCase));
}

void StringPairArray::remove(const int index)
{
	keys.remove(index);
	values.remove(index);
}

void StringPairArray::setIgnoresCase(const bool shouldIgnoreCase)
{
	ignoreCase = shouldIgnoreCase;
}

void StringPairArray::minimiseStorageOverheads()
{
	keys.minimiseStorageOverheads();
	values.minimiseStorageOverheads();
}

}