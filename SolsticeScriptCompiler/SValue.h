#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace Solstice
{
	class GarbageCollector;
	struct GarbageCollectorState;
	class GCRoot;
	struct GCObject;
	struct VMString;
	struct VMVector;

	enum class ValueType
	{
		Nil,
		Boolean,
		Number,
		String,
		Array
	};

	class SValue
	{
	public:
		SValue() = default;

		static SValue Boolean(bool value);
		static SValue Number(double value);
		ValueType Type() const noexcept;
		bool IsNil() const noexcept;
		bool IsTruthy() const noexcept;
		bool AsBoolean() const;
		double AsNumber() const;
		const VMString* AsString() const;
		const VMVector* AsArray() const;
		std::string ToString() const;

	private:
		using Storage = std::variant<std::monostate, bool, double, VMString*, VMVector*>;

		explicit SValue(Storage value);
		static SValue String(VMString* value);
		static SValue Array(VMVector* value);
		SValue WithoutRoot() const;
		VMString* MutableString() const;
		VMVector* MutableArray() const;

		Storage mValue;
		std::shared_ptr<GCRoot> mRoot;

		friend class GarbageCollector;
		friend class SVirtualMachine;
	};

	struct GCObject
	{
		virtual ~GCObject() = default;
		virtual void Trace(GarbageCollector& collector) = 0;

		bool marked = false;
	};

	struct VMString final : public GCObject
	{
		explicit VMString(std::string value);
		void Trace(GarbageCollector& collector) override;

		std::string str;
	};

	struct VMVector final : public GCObject
	{
		explicit VMVector(std::vector<SValue> values = {});
		void Trace(GarbageCollector& collector) override;

		std::vector<SValue> arr;
	};

	class GarbageCollector
	{
	public:
		GarbageCollector();

		SValue CreateString(std::string value);
		SValue CreateArray(std::vector<SValue> values = {});
		SValue RootValue(const SValue& value);

		void Collect(std::span<const SValue> stackRoots, std::span<const SValue> globalRoots);
		void MarkValue(const SValue& value);

		std::size_t ObjectCount() const noexcept;

	private:
		void MarkObject(GCObject* object);

		std::shared_ptr<GarbageCollectorState> mState;
	};
}