// Copyright (c) YugaByte, Inc.
//
// This file contains the YQLValue class that represents YQL values.

#ifndef YB_COMMON_YQL_VALUE_H
#define YB_COMMON_YQL_VALUE_H

#include <stdint.h>

#include "yb/common/yql_protocol.pb.h"
#include "yb/util/decimal.h"
#include "yb/util/net/inetaddress.h"
#include "yb/util/timestamp.h"
#include "yb/common/yql_type.h"
#include "yb/util/uuid.h"

namespace yb {

// An abstract class that defines a YQL value interface to support different implementations
// for in-memory / serialization trade-offs.
class YQLValue {
 public:
  // The value type.
  typedef YQLValuePB::ValueCase InternalType;

  virtual ~YQLValue();

  //-----------------------------------------------------------------------------------------
  // Interfaces to be implemented.

  // Return the value's type.
  virtual InternalType type() const = 0;

  //------------------------------------ Nullness methods -----------------------------------
  // Is the value null?
  virtual bool IsNull() const = 0;
  // Set the value to null.
  virtual void SetNull() = 0;

  //----------------------------------- get value methods -----------------------------------
  // Get different datatype values. CHECK failure will result if the value stored is not of the
  // expected datatype or the value is null.
  virtual int8_t int8_value() const = 0;
  virtual int16_t int16_value() const = 0;
  virtual int32_t int32_value() const = 0;
  virtual int64_t int64_value() const = 0;
  virtual float float_value() const = 0;
  virtual double double_value() const = 0;
  virtual const std::string& decimal_value() const = 0;
  virtual bool bool_value() const = 0;
  virtual const std::string& string_value() const = 0;
  virtual Timestamp timestamp_value() const = 0;
  virtual const std::string& binary_value() const = 0;
  virtual InetAddress inetaddress_value() const = 0;
  virtual const YQLMapValuePB map_value() const = 0;
  virtual const YQLSeqValuePB set_value() const = 0;
  virtual const YQLSeqValuePB list_value() const = 0;
  virtual Uuid uuid_value() const = 0;
  virtual Uuid timeuuid_value() const = 0;

  //----------------------------------- set value methods -----------------------------------
  // Set different datatype values.
  virtual void set_int8_value(int8_t val) = 0;
  virtual void set_int16_value(int16_t val) = 0;
  virtual void set_int32_value(int32_t val) = 0;
  virtual void set_int64_value(int64_t val) = 0;
  virtual void set_float_value(float val) = 0;
  virtual void set_double_value(double val) = 0;
  virtual void set_decimal_value(const std::string& val) = 0;
  virtual void set_bool_value(bool val) = 0;
  virtual void set_string_value(const std::string& val) = 0;
  virtual void set_string_value(const char* val) = 0;
  virtual void set_string_value(const char* val, size_t size) = 0;
  virtual void set_timestamp_value(const Timestamp& val) = 0;
  virtual void set_timestamp_value(int64_t val) = 0;
  virtual void set_binary_value(const std::string& val) = 0;
  virtual void set_binary_value(const void* val, size_t size) = 0;
  virtual void set_inetaddress_value(const InetAddress& val) = 0;
  virtual void set_uuid_value(const Uuid& val) = 0;
  virtual void set_timeuuid_value(const Uuid& val) = 0;

  //--------------------------------- mutable value methods ---------------------------------
  virtual std::string* mutable_decimal_value() = 0;
  virtual std::string* mutable_string_value() = 0;
  virtual std::string* mutable_binary_value() = 0;

  // for collections the setters just allocate the message and set the correct value type
  virtual void set_map_value() = 0;
  virtual void set_set_value() = 0;
  virtual void set_list_value() = 0;
  // the `add_foo` methods append a new element to the corresponding collection and return a pointer
  // so that its value can be set by the caller
  virtual YQLValuePB* add_map_key() = 0;
  virtual YQLValuePB* add_map_value() = 0;
  virtual YQLValuePB* add_set_elem() = 0;
  virtual YQLValuePB* add_list_elem() = 0;

  //----------------------------------- assignment methods ----------------------------------
  YQLValue& operator=(const YQLValuePB& other) {
    Assign(other);
    return *this;
  }

  YQLValue& operator=(YQLValuePB&& other) {
    AssignMove(std::move(other));
    return *this;
  }

  virtual void Assign(const YQLValuePB& other) = 0;
  virtual void AssignMove(YQLValuePB&& other) = 0;

  //-----------------------------------------------------------------------------------------
  // Methods provided by this abstract class.

  //----------------------------------- comparison methods -----------------------------------
  virtual bool Comparable(const YQLValue& other) const {
    return type() == other.type() || EitherIsNull(other);
  }
  virtual bool BothNotNull(const YQLValue& other) const { return !IsNull() && !other.IsNull(); }
  virtual bool EitherIsNull(const YQLValue& other) const { return IsNull() || other.IsNull(); }
  virtual int CompareTo(const YQLValue& other) const;
  virtual bool operator <(const YQLValue& v) const { return BothNotNull(v) && CompareTo(v) < 0; }
  virtual bool operator >(const YQLValue& v) const { return BothNotNull(v) && CompareTo(v) > 0; }
  virtual bool operator <=(const YQLValue& v) const { return BothNotNull(v) && CompareTo(v) <= 0; }
  virtual bool operator >=(const YQLValue& v) const { return BothNotNull(v) && CompareTo(v) >= 0; }
  virtual bool operator ==(const YQLValue& v) const { return BothNotNull(v) && CompareTo(v) == 0; }
  virtual bool operator !=(const YQLValue& v) const { return BothNotNull(v) && CompareTo(v) != 0; }

  //----------------------------- serializer / deserializer ---------------------------------
  virtual void Serialize(const std::shared_ptr<YQLType>& yql_type,
                         const YQLClient& client,
                         faststring* buffer) const;
  virtual CHECKED_STATUS Deserialize(const std::shared_ptr<YQLType>& yql_type,
                                     const YQLClient& client,
                                     Slice* data);

  //------------------------------------ debug string ---------------------------------------
  // Return a string for debugging.
  virtual std::string ToString() const;

  //-----------------------------------------------------------------------------------------
  // The following static functions provide YQLValue-equivalent interfaces for use with an
  // existing YQLValuePB without wrapping it as a YQLValue.
  //-----------------------------------------------------------------------------------------

  // The value's type.
  static InternalType type(const YQLValuePB& v) { return v.value_case(); }

  //------------------------------------ Nullness methods -----------------------------------
  // Is the value null?
  static bool IsNull(const YQLValuePB& v) { return v.value_case() == YQLValuePB::VALUE_NOT_SET; }
  // Set the value to null.
  static void SetNull(YQLValuePB* v);

  //----------------------------------- get value methods -----------------------------------
  // Get different datatype values. CHECK failure will result if the value stored is not of the
  // expected datatype or the value is null.
#define YQL_GET_VALUE(v, type, field)                                   \
  do { CHECK(v.has_##field()); return static_cast<type>(v.field()); } while (0)

  static int8_t int8_value(const YQLValuePB& v) { YQL_GET_VALUE(v, int8_t, int8_value); }
  static int16_t int16_value(const YQLValuePB& v) { YQL_GET_VALUE(v, int16_t, int16_value); }
  static int32_t int32_value(const YQLValuePB& v) { YQL_GET_VALUE(v, int32_t, int32_value); }
  static int64_t int64_value(const YQLValuePB& v) { YQL_GET_VALUE(v, int64_t, int64_value); }
  static float float_value(const YQLValuePB& v) { YQL_GET_VALUE(v, float, float_value); }
  static double double_value(const YQLValuePB& v) { YQL_GET_VALUE(v, double, double_value); }
  static const std::string& decimal_value(const YQLValuePB& v) {
    CHECK(v.has_decimal_value());
    return v.decimal_value();
  }
  static bool bool_value(const YQLValuePB& v) { YQL_GET_VALUE(v, bool, bool_value); }
  static const std::string& string_value(const YQLValuePB& v) {
    CHECK(v.has_string_value());
    return v.string_value();
  }
  static Timestamp timestamp_value(const YQLValuePB& v) {
    CHECK(v.has_timestamp_value());
    return Timestamp(v.timestamp_value());
  }
  static const std::string& binary_value(const YQLValuePB& v) {
    CHECK(v.has_binary_value());
    return v.binary_value();
  }
  static YQLMapValuePB map_value(const YQLValuePB& v) {
    CHECK(v.has_map_value());
    return v.map_value();
  }
  static YQLSeqValuePB set_value(const YQLValuePB& v) {
    CHECK(v.has_set_value());
    return v.set_value();
  }
  static YQLSeqValuePB list_value(const YQLValuePB& v) {
    CHECK(v.has_list_value());
    return v.list_value();
  }

  static InetAddress inetaddress_value(const YQLValuePB& v) {
    CHECK(v.has_inetaddress_value());
    InetAddress addr;
    CHECK_OK(addr.FromBytes(v.inetaddress_value()));
    return addr;
  }

  static Uuid uuid_value(const YQLValuePB& v) {
    CHECK(v.has_uuid_value());
    Uuid uuid;
    CHECK_OK(uuid.FromBytes(v.uuid_value()));
    return uuid;
  }

  static Uuid timeuuid_value(const YQLValuePB& v) {
    CHECK(v.has_timeuuid_value());
    Uuid timeuuid;
    CHECK_OK(timeuuid.FromBytes(v.timeuuid_value()));
    CHECK_OK(timeuuid.IsTimeUuid());
    return timeuuid;
  }

#undef YQL_GET_VALUE

  //----------------------------------- set value methods -----------------------------------
  // Set different datatype values.
  static void set_int8_value(const int8_t val, YQLValuePB *v) { v->set_int8_value(val); }
  static void set_int16_value(const int16_t val, YQLValuePB *v) { v->set_int16_value(val); }
  static void set_int32_value(const int32_t val, YQLValuePB *v) { v->set_int32_value(val); }
  static void set_int64_value(const int64_t val, YQLValuePB *v) { v->set_int64_value(val); }
  static void set_float_value(const float val, YQLValuePB *v) { v->set_float_value(val); }
  static void set_double_value(const double val, YQLValuePB *v) { v->set_double_value(val); }
  static void set_decimal_value(const std::string& val, YQLValuePB *v) {
    v->set_decimal_value(val);
  }
  static void set_decimal_value(const char* val, const size_t size, YQLValuePB *v) {
    v->set_decimal_value(val, size);
  }
  static void set_bool_value(const bool val, YQLValuePB *v) { v->set_bool_value(val); }
  static void set_string_value(const std::string& val, YQLValuePB *v) { v->set_string_value(val); }
  static void set_string_value(const char* val, YQLValuePB *v) { v->set_string_value(val); }
  static void set_string_value(const char* val, const size_t size, YQLValuePB *v) {
    v->set_string_value(val, size);
  }
  static void set_timestamp_value(const Timestamp& val, YQLValuePB *v) {
    v->set_timestamp_value(val.ToInt64());
  }

  static void set_inetaddress_value(const InetAddress& val, YQLValuePB *v) {
    std::string bytes;
    CHECK_OK(val.ToBytes(&bytes));
    v->set_inetaddress_value(bytes);
  }

  static void set_uuid_value(const Uuid& val, YQLValuePB *v) {
    std::string bytes;
    CHECK_OK(val.ToBytes(&bytes));
    v->set_uuid_value(bytes);
  }

  static void set_timeuuid_value(const Uuid& val, YQLValuePB *v) {
    CHECK_OK(val.IsTimeUuid());
    std::string bytes;
    CHECK_OK(val.ToBytes(&bytes));
    v->set_timeuuid_value(bytes);
  }

  static void set_timestamp_value(const int64_t val, YQLValuePB *v) { v->set_timestamp_value(val); }
  static void set_binary_value(const std::string& val, YQLValuePB *v) { v->set_binary_value(val); }
  static void set_binary_value(const void* val, const size_t size, YQLValuePB *v) {
    v->set_binary_value(static_cast<const char *>(val), size);
  }

  // For collections, the call to `mutable_foo` takes care of setting the correct type to `foo`
  // internally and allocating the message if needed
  static void set_map_value(YQLValuePB *v) {v->mutable_map_value();}
  static void set_set_value(YQLValuePB *v) {v->mutable_set_value();}
  static void set_list_value(YQLValuePB *v) {v->mutable_list_value();}

  // To extend/construct collections we return freshly allocated elements for the caller to set.
  static YQLValuePB* add_map_key(YQLValuePB *v) {
    return v->mutable_map_value()->add_keys();
  }
  static YQLValuePB* add_map_value(YQLValuePB *v) {
    return v->mutable_map_value()->add_values();
  }
  static YQLValuePB* add_set_elem(YQLValuePB *v) {
    return v->mutable_set_value()->add_elems();
  }
  static YQLValuePB* add_list_elem(YQLValuePB *v) {
    return v->mutable_list_value()->add_elems();
  }

  //--------------------------------- mutable value methods ----------------------------------
  static std::string* mutable_decimal_value(YQLValuePB *v) { return v->mutable_decimal_value(); }
  static std::string* mutable_string_value(YQLValuePB *v) { return v->mutable_string_value(); }
  static std::string* mutable_binary_value(YQLValuePB *v) { return v->mutable_binary_value(); }

  //----------------------------------- comparison methods -----------------------------------
  static bool Comparable(const YQLValuePB& lhs, const YQLValuePB& rhs) {
    return lhs.value_case() == rhs.value_case() || EitherIsNull(lhs, rhs);
  }
  static bool BothNotNull(const YQLValuePB& lhs, const YQLValuePB& rhs) {
    return !IsNull(lhs) && !IsNull(rhs);
  }
  static bool EitherIsNull(const YQLValuePB& lhs, const YQLValuePB& rhs) {
    return IsNull(lhs) || IsNull(rhs);
  }
  static int CompareTo(const YQLValuePB& lhs, const YQLValuePB& rhs);

  static bool Comparable(const YQLValuePB& lhs, const YQLValue& rhs) {
    return type(lhs) == rhs.type() || EitherIsNull(lhs, rhs);
  }
  static bool BothNotNull(const YQLValuePB& lhs, const YQLValue& rhs) {
    return !IsNull(lhs) && !rhs.IsNull();
  }
  static bool EitherIsNull(const YQLValuePB& lhs, const YQLValue& rhs) {
    return IsNull(lhs) || rhs.IsNull();
  }
  static int CompareTo(const YQLValuePB& lhs, const YQLValue& rhs);

 private:
  // Deserialize a CQL number (8, 16, 32 and 64-bit integer). <num_type> is the parsed integer type.
  // <converter> converts the number from network byte-order to machine order and <data_type>
  // is the coverter's return type. The converter's return type <data_type> is unsigned while
  // <num_type> may be signed or unsigned. <setter> sets the value in YQLValue.
  template<typename num_type, typename data_type>
  CHECKED_STATUS CQLDeserializeNum(
      size_t len, data_type (*converter)(const void*), void (YQLValue::*setter)(num_type),
      Slice* data) {
    num_type value = 0;
    RETURN_NOT_OK(CQLDecodeNum(len, converter, data, &value));
    (this->*setter)(value);
    return Status::OK();
  }

  // Deserialize a CQL floating point number (float or double). <float_type> is the parsed floating
  // point type. <converter> converts the number from network byte-order to machine order and
  // <data_type> is the coverter's return type. The converter's return type <data_type> is an
  // integer type. <setter> sets the value in YQLValue.
  template<typename float_type, typename data_type>
  CHECKED_STATUS CQLDeserializeFloat(
      size_t len, data_type (*converter)(const void*), void (YQLValue::*setter)(float_type),
      Slice* data) {
    float_type value = 0.0;
    RETURN_NOT_OK(CQLDecodeFloat(len, converter, data, &value));
    (this->*setter)(value);
    return Status::OK();
  }
};

//-------------------------------------------------------------------------------------------
// YQLValuePB operators
bool operator <(const YQLValuePB& lhs, const YQLValuePB& rhs);
bool operator >(const YQLValuePB& lhs, const YQLValuePB& rhs);
bool operator <=(const YQLValuePB& lhs, const YQLValuePB& rhs);
bool operator >=(const YQLValuePB& lhs, const YQLValuePB& rhs);
bool operator ==(const YQLValuePB& lhs, const YQLValuePB& rhs);
bool operator !=(const YQLValuePB& lhs, const YQLValuePB& rhs);

bool operator <(const YQLValuePB& lhs, const YQLValue& rhs);
bool operator >(const YQLValuePB& lhs, const YQLValue& rhs);
bool operator <=(const YQLValuePB& lhs, const YQLValue& rhs);
bool operator >=(const YQLValuePB& lhs, const YQLValue& rhs);
bool operator ==(const YQLValuePB& lhs, const YQLValue& rhs);
bool operator !=(const YQLValuePB& lhs, const YQLValue& rhs);

//-------------------------------------------------------------------------------------------
// A class that implements YQLValue interface using YQLValuePB.
class YQLValueWithPB : public YQLValue, public YQLValuePB {
 public:
  YQLValueWithPB() { }
  explicit YQLValueWithPB(const YQLValuePB& val) { *mutable_value() = val; }
  virtual ~YQLValueWithPB() override { }

  virtual InternalType type() const override { return YQLValue::type(value()); }

  const YQLValuePB& value() const { return *static_cast<const YQLValuePB*>(this); }
  YQLValuePB* mutable_value() { return static_cast<YQLValuePB*>(this); }

  //------------------------------------ Nullness methods -----------------------------------
  virtual bool IsNull() const override { return YQLValue::IsNull(value()); }
  virtual void SetNull() override { YQLValue::SetNull(mutable_value()); }

  //----------------------------------- get value methods -----------------------------------
  virtual int8_t int8_value() const override { return YQLValue::int8_value(value()); }
  virtual int16_t int16_value() const override { return YQLValue::int16_value(value()); }
  virtual int32_t int32_value() const override { return YQLValue::int32_value(value()); }
  virtual int64_t int64_value() const override { return YQLValue::int64_value(value()); }
  virtual float float_value() const override { return YQLValue::float_value(value()); }
  virtual double double_value() const override { return YQLValue::double_value(value()); }
  virtual const std::string& decimal_value() const override {
    return YQLValue::decimal_value(value());
  }
  virtual bool bool_value() const override { return YQLValue::bool_value(value()); }
  virtual const string& string_value() const override { return YQLValue::string_value(value()); }
  virtual Timestamp timestamp_value() const override { return YQLValue::timestamp_value(value()); }
  virtual const std::string& binary_value() const override {
    return YQLValue::binary_value(value());
  }
  virtual InetAddress inetaddress_value() const override {
    return YQLValue::inetaddress_value(value());
  }
  virtual const YQLMapValuePB map_value() const override { return YQLValue::map_value(value()); }
  virtual const YQLSeqValuePB set_value() const override { return YQLValue::set_value(value()); }
  virtual const YQLSeqValuePB list_value() const override { return YQLValue::list_value(value()); }
  virtual Uuid uuid_value() const override { return YQLValue::uuid_value(value()); }
  virtual Uuid timeuuid_value() const override { return YQLValue::timeuuid_value(value()); }

  //----------------------------------- set value methods -----------------------------------
  virtual void set_int8_value(const int8_t val) override {
    YQLValue::set_int8_value(val, mutable_value());
  }
  virtual void set_int16_value(const int16_t val) override {
    YQLValue::set_int16_value(val, mutable_value());
  }
  virtual void set_int32_value(const int32_t val) override {
    YQLValue::set_int32_value(val, mutable_value());
  }
  virtual void set_int64_value(const int64_t val) override {
    YQLValue::set_int64_value(val, mutable_value());
  }
  virtual void set_float_value(const float val) override {
    YQLValue::set_float_value(val, mutable_value());
  }
  virtual void set_double_value(const double val) override {
    YQLValue::set_double_value(val, mutable_value());
  }
  virtual void set_decimal_value(const std::string& val) override {
    YQLValue::set_decimal_value(val, mutable_value());
  }
  virtual void set_bool_value(const bool val) override {
    YQLValue::set_bool_value(val, mutable_value());
  }
  virtual void set_string_value(const string& val) override {
    YQLValue::set_string_value(val, mutable_value());
  }
  virtual void set_string_value(const char* val) override {
    YQLValue::set_string_value(val, mutable_value());
  }
  virtual void set_string_value(const char* val, const size_t size) override {
    YQLValue::set_string_value(val, size, mutable_value());
  }
  virtual void set_timestamp_value(const Timestamp& val) override {
    YQLValue::set_timestamp_value(val, mutable_value());
  }
  virtual void set_inetaddress_value(const InetAddress& val) override {
    YQLValue::set_inetaddress_value(val, mutable_value());
  }
  virtual void set_timestamp_value(const int64_t val) override {
    YQLValue::set_timestamp_value(val, mutable_value());
  }
  virtual void set_uuid_value(const Uuid& val) override {
    YQLValue::set_uuid_value(val, mutable_value());
  }
  virtual void set_timeuuid_value(const Uuid& val) override {
    YQLValue::set_timeuuid_value(val, mutable_value());
  }
  virtual void set_binary_value(const std::string& val) override {
    YQLValue::set_binary_value(val, mutable_value());
  }
  virtual void set_binary_value(const void* val, const size_t size) override {
    YQLValue::set_binary_value(val, size, mutable_value());
  }
  virtual void set_map_value() override {
    YQLValue::set_map_value(mutable_value());
  }
  virtual void set_set_value() override {
    YQLValue::set_set_value(mutable_value());
  }
  virtual void set_list_value() override {
    YQLValue::set_list_value(mutable_value());
  }
  virtual YQLValuePB* add_map_key() override {
    return YQLValue::add_map_key(mutable_value());
  }
  virtual YQLValuePB* add_map_value() override {
    return YQLValue::add_map_value(mutable_value());
  }
  virtual YQLValuePB* add_set_elem() override {
    return YQLValue::add_set_elem(mutable_value());
  }
  virtual YQLValuePB* add_list_elem() override {
    return YQLValue::add_list_elem(mutable_value());
  }

  //--------------------------------- mutable value methods ---------------------------------
  virtual std::string* mutable_decimal_value() override {
    return YQLValue::mutable_decimal_value(mutable_value());
  }
  virtual std::string* mutable_string_value() override {
    return YQLValue::mutable_string_value(mutable_value());
  }
  virtual std::string* mutable_binary_value() override {
    return YQLValue::mutable_binary_value(mutable_value());
  }

  //----------------------------------- assignment methods ----------------------------------
  virtual void Assign(const YQLValuePB& other) override {
    CopyFrom(other);
  }
  virtual void AssignMove(YQLValuePB&& other) override {
    Swap(&other);
  }
};

} // namespace yb

#endif // YB_COMMON_YQL_VALUE_H
