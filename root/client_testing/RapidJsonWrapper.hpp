#pragma once

#include <string>
#include <memory>
#include <rapidjson/writer.h>
#include <rapidjson/document.h>

namespace duobei {
class JsonArrayBuilder;
class JsonAssigner {
public:
	enum Null {
		null
	};
	explicit JsonAssigner(rapidjson::Writer<rapidjson::StringBuffer>* writer) : _pWriter(writer) { }

	int operator=(int value) {
		_pWriter->Int(value);
		return value;
	}

	double operator=(double value) {
		_pWriter->Double(value);
		return value;
	}

	bool operator=(bool b) {
		_pWriter->Bool(b);
		return b;
	}

	int64_t operator=(int64_t v) {
		_pWriter->Int64(v);
		return v;
	}

	std::string& operator=(std::string& s) {
		_pWriter->String(s.c_str());
		return s;
	}

	const std::string& operator=(const std::string& s) {
		_pWriter->String(s.c_str());
		return s;
	}

	const char* operator=(const char* s) {
		_pWriter->String(s);
		return s;
	}

	char* operator=(char* s) {
		_pWriter->String(s);
		return s;
	}

	void* operator=(Null n) {
		_pWriter->Null();
		return nullptr;
	}

	rapidjson::Writer<rapidjson::StringBuffer>* _pWriter;
};

class JsonObjectBuilder {
	struct Private {
		explicit Private(rapidjson::Writer<rapidjson::StringBuffer>* writer): _pWriter(writer) {
			_pWriter->StartObject();
		}
		~Private() {
			_pWriter->EndObject();
		}

		rapidjson::Writer<rapidjson::StringBuffer>* _pWriter;
	};
	std::shared_ptr<Private> _private;

public:
	explicit JsonObjectBuilder(rapidjson::Writer<rapidjson::StringBuffer>*writer) : _private(nullptr) {
		_private = std::shared_ptr<Private>(new Private(writer));
	}

	JsonAssigner operator[](const std::string& key) {
		_private->_pWriter->Key(key.c_str());
		return JsonAssigner(_private->_pWriter);
	}

	JsonAssigner operator[](const char* key) {
		_private->_pWriter->Key(key);
		return JsonAssigner(_private->_pWriter);
	}

	JsonObjectBuilder addChildObject(const std::string& key) {
		_private->_pWriter->Key(key.c_str());
		return JsonObjectBuilder(_private->_pWriter);
	}

	JsonObjectBuilder addChildObject(const char* key) {
		_private->_pWriter->Key(key);
		return JsonObjectBuilder(_private->_pWriter);
	}

	inline JsonArrayBuilder addChildArray(const std::string& key);
	inline JsonArrayBuilder addChildArray(const char* key);
};

class JsonArrayBuilder {
	struct Private {
		explicit Private(rapidjson::Writer<rapidjson::StringBuffer>* writer): _pWriter(writer) {
			_pWriter->StartArray();
		}
		~Private() {
			_pWriter->EndArray();
		}

		rapidjson::Writer<rapidjson::StringBuffer>* _pWriter;
	};
	std::shared_ptr<Private> _private;

public:
	explicit JsonArrayBuilder(rapidjson::Writer<rapidjson::StringBuffer>*writer) : _private() {
		_private = std::shared_ptr<Private>(new Private(writer));
	}

	JsonObjectBuilder addChildObject() {
		return JsonObjectBuilder(_private->_pWriter);
	}

	JsonArrayBuilder addChildArray() {
		return JsonArrayBuilder(_private->_pWriter);
	}

	JsonArrayBuilder& addString(const char* s) {
		_private->_pWriter->String(s);
		return *this;
	}

	JsonArrayBuilder& addString(const std::string& s) {
		_private->_pWriter->String(s.c_str());
		return *this;
	}

	JsonArrayBuilder& addInt(int v) {
		_private->_pWriter->Int(v);
		return *this;
	}

	JsonArrayBuilder& addInt64(int64_t v) {
		_private->_pWriter->Int64(v);
		return *this;
	}

	JsonArrayBuilder& addBool(bool v) {
		_private->_pWriter->Bool(v);
		return *this;
	}

	JsonArrayBuilder& addDouble(double v) {
		_private->_pWriter->Double(v);
		return *this;
	}

	JsonArrayBuilder& addNull() {
		_private->_pWriter->Null();
		return *this;
	}
};

JsonArrayBuilder JsonObjectBuilder::addChildArray(const std::string& key)
{
	_private->_pWriter->Key(key.c_str());
	return JsonArrayBuilder(_private->_pWriter);
}

JsonArrayBuilder JsonObjectBuilder::addChildArray(const char* key)
{
	_private->_pWriter->Key(key);
	return JsonArrayBuilder(_private->_pWriter);
}

class JsonBuilder {
public:
	JsonBuilder() : _result(), _writer(_result) { }

	JsonBuilder(const JsonBuilder&) = delete;
	JsonBuilder(JsonBuilder&&) = delete;
	JsonBuilder& operator=(const JsonBuilder&) = delete;
	JsonBuilder& operator=(JsonBuilder&&) = delete;

    JsonArrayBuilder array() {
		return JsonArrayBuilder(&_writer);
	}
	JsonObjectBuilder object() {
		return JsonObjectBuilder(&_writer);
	}

	std::string toString() {
		return _result.GetString();
	}

	void clear() {
		_result.Clear();
		_writer.Reset(_result);
	}

private:
	rapidjson::StringBuffer _result;
	rapidjson::Writer<rapidjson::StringBuffer> _writer;
};

}  // namespace duobei
