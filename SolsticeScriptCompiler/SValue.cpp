#include "SValue.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <utility>

namespace Solstice
{
	struct GarbageCollectorState
	{
		std::vector<std::unique_ptr<GCObject>> objects;
		std::unordered_map<GCObject*, std::size_t> externalRoots;
	};

	class GCRoot
	{
	public:
		GCRoot(std::shared_ptr<GarbageCollectorState> state, GCObject* object)
			: mState(std::move(state)),
			  mObject(object)
		{
			++mState->externalRoots[mObject];
		}

		~GCRoot()
		{
			auto root = mState->externalRoots.find(mObject);
			if(root == mState->externalRoots.end())
				return;

			if(--root->second == 0)
				mState->externalRoots.erase(root);
		}

	private:
		std::shared_ptr<GarbageCollectorState> mState;
		GCObject* mObject = nullptr;
	};

	SValue::SValue(Storage value)
		: mValue(std::move(value))
	{
	}

	SValue SValue::Boolean(bool value)
	{
		return SValue(value);
	}

	SValue SValue::Number(double value)
	{
		return SValue(value);
	}

	SValue SValue::String(VMString* value)
	{
		return SValue(value);
	}

	SValue SValue::Array(VMVector* value)
	{
		return SValue(value);
	}

	SValue SValue::WithoutRoot() const
	{
		SValue value = *this;
		value.mRoot.reset();
		return value;
	}

	ValueType SValue::Type() const noexcept
	{
		switch(mValue.index())
		{
		case 1:
			return ValueType::Boolean;
		case 2:
			return ValueType::Number;
		case 3:
			return ValueType::String;
		case 4:
			return ValueType::Array;
		default:
			return ValueType::Nil;
		}
	}

	bool SValue::IsNil() const noexcept
	{
		return std::holds_alternative<std::monostate>(mValue);
	}

	bool SValue::IsTruthy() const noexcept
	{
		if(IsNil())
			return false;

		if(const bool* value = std::get_if<bool>(&mValue))
			return *value;

		if(const double* value = std::get_if<double>(&mValue))
			return *value != 0.0;

		return true;
	}

	bool SValue::AsBoolean() const
	{
		return std::get<bool>(mValue);
	}

	double SValue::AsNumber() const
	{
		return std::get<double>(mValue);
	}

	const VMString* SValue::AsString() const
	{
		return std::get<VMString*>(mValue);
	}

	const VMVector* SValue::AsArray() const
	{
		return std::get<VMVector*>(mValue);
	}

	VMString* SValue::MutableString() const
	{
		return std::get<VMString*>(mValue);
	}

	VMVector* SValue::MutableArray() const
	{
		return std::get<VMVector*>(mValue);
	}

	std::string SValue::ToString() const
	{
		switch(Type())
		{
		case ValueType::Nil:
			return "nil";
		case ValueType::Boolean:
			return AsBoolean() ? "true" : "false";
		case ValueType::Number:
			{
				std::ostringstream output;
				output << std::setprecision(15) << AsNumber();
				return output.str();
			}
		case ValueType::String:
			return AsString()->str;
		case ValueType::Array:
			return "<array>";
		}

		return "nil";
	}

	VMString::VMString(std::string value)
		: str(std::move(value))
	{
	}

	void VMString::Trace(GarbageCollector&)
	{
	}

	VMVector::VMVector(std::vector<SValue> values)
		: arr(std::move(values))
	{
	}

	void VMVector::Trace(GarbageCollector& collector)
	{
		for(const SValue& value : arr)
			collector.MarkValue(value);
	}

	GarbageCollector::GarbageCollector()
		: mState(std::make_shared<GarbageCollectorState>())
	{
	}

	SValue GarbageCollector::CreateString(std::string value)
	{
		auto object = std::make_unique<VMString>(std::move(value));
		VMString* result = object.get();
		mState->objects.push_back(std::move(object));
		return SValue::String(result);
	}

	SValue GarbageCollector::CreateArray(std::vector<SValue> values)
	{
		auto object = std::make_unique<VMVector>(std::move(values));
		VMVector* result = object.get();
		mState->objects.push_back(std::move(object));
		return SValue::Array(result);
	}

	SValue GarbageCollector::RootValue(const SValue& value)
	{
		SValue rootedValue = value;
		GCObject* object = nullptr;
		if(value.Type() == ValueType::String)
			object = value.MutableString();
		else if(value.Type() == ValueType::Array)
			object = value.MutableArray();

		if(object != nullptr)
			rootedValue.mRoot = std::make_shared<GCRoot>(mState, object);

		return rootedValue;
	}

	void GarbageCollector::Collect(
		std::span<const SValue> stackRoots,
		std::span<const SValue> globalRoots)
	{
		for(const SValue& value : stackRoots)
			MarkValue(value);

		for(const SValue& value : globalRoots)
			MarkValue(value);

		for(const auto& [object, count] : mState->externalRoots)
		{
			if(count != 0)
				MarkObject(object);
		}

		std::erase_if(
			mState->objects,
			[](const std::unique_ptr<GCObject>& object)
			{
				return !object->marked;
			});

		for(const std::unique_ptr<GCObject>& object : mState->objects)
			object->marked = false;
	}

	void GarbageCollector::MarkValue(const SValue& value)
	{
		if(value.Type() == ValueType::String)
			MarkObject(value.MutableString());
		else if(value.Type() == ValueType::Array)
			MarkObject(value.MutableArray());
	}

	std::size_t GarbageCollector::ObjectCount() const noexcept
	{
		return mState->objects.size();
	}

	void GarbageCollector::MarkObject(GCObject* object)
	{
		if(object == nullptr || object->marked)
			return;

		object->marked = true;
		object->Trace(*this);
	}
}
