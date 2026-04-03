# Plan: String Interning Optimization

## Objective
Implement String Interning to eliminate redundant string allocations and transform $O(N)$ string comparisons into $O(1)$ pointer comparisons.

## Key Files
- `include/pome_gc.h`: Add `stringPool_` member.
- `src/pome_gc.cpp`: Implement pool cleanup in `sweep`.
- `include/pome_value.h`: Add `hash_` to `PomeString`.
- `src/pome_value.cpp`: Update `operator==` and `hash()` for `PomeValue`.
- `include/pome_gc_impl.h`: Specialize `allocate<PomeString>`.

## Implementation Steps

### 1. PomeString & PomeValue Updates
- Add `size_t hash_` to `PomeString`.
- Initialize `hash_` in `PomeString` constructor using `std::hash<std::string>`.
- Update `PomeValue::operator==`: If both are strings, return `asObject() == other.asObject()`.
- Update `PomeValue::hash()`: If string, return `asStringObject()->hash_`.

### 2. GarbageCollector Updates
- Add `std::unordered_map<std::string, PomeString*> stringPool_` to `GarbageCollector`.
- In `GarbageCollector::sweep`, iterate through objects being deleted. If type is `STRING`, remove from `stringPool_`.

### 3. Allocation Specialization
- In `include/pome_gc_impl.h`, provide a specialization for `template<> PomeString* GarbageCollector::allocate<PomeString>(std::string&& value)`.
- This specialization will check the pool before performing a new allocation.

## Verification
- Run `test/unit_tests/test_features.pome`.
- Run `tools/pome_bench.py` and compare Word Count and JSON Parse ratios.
