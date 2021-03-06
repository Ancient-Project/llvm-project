//===- InferTypeOpInterface.h - Infer Type Interfaces -----------*- C++ -*-===//
//
// Part of the MLIR Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the definitions of the infer op interfaces defined in
// `InferTypeOpInterface.td`.
//
//===----------------------------------------------------------------------===//

#ifndef MLIR_ANALYSIS_INFERTYPEOPINTERFACE_H_
#define MLIR_ANALYSIS_INFERTYPEOPINTERFACE_H_

#include "mlir/IR/Attributes.h"
#include "mlir/IR/Location.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/Support/LLVM.h"
#include "llvm/ADT/SmallVector.h"

namespace mlir {

/// ShapedTypeComponents that represents the components of a ShapedType.
/// The components consist of
///  - A ranked or unranked shape with the dimension specification match those
///    of ShapeType's getShape() (e.g., dynamic dimension represented using
///    ShapedType::kDynamicSize)
///  - A element type, may be unset (nullptr)
///  - A attribute, may be unset (nullptr)
/// Used by ShapedType type inferences.
class ShapedTypeComponents {
  /// Internal storage type for shape.
  using ShapeStorageT = SmallVector<int64_t, 3>;

public:
  /// Default construction is an unranked shape.
  ShapedTypeComponents() : ranked(false), elementType(nullptr), attr(nullptr){};

  template <typename Arg, typename = typename std::enable_if_t<
                              std::is_constructible<ShapeStorageT, Arg>::value>>
  ShapedTypeComponents(Arg &&arg, Type elementType = nullptr,
                       Attribute attr = nullptr)
      : dims(std::forward<Arg>(arg)), ranked(true), elementType(elementType),
        attr(attr) {}
  ShapedTypeComponents(ArrayRef<int64_t> vec, Type elementType = nullptr,
                       Attribute attr = nullptr)
      : dims(vec.begin(), vec.end()), ranked(true), elementType(elementType),
        attr(attr) {}

  /// Return the dimensions of the shape.
  /// Requires: shape is ranked.
  ArrayRef<int64_t> getDims() const {
    assert(ranked && "requires ranked shape");
    return dims;
  }

  /// Return whether the shape has a rank.
  bool hasRank() const { return ranked; };

  /// Return the element type component.
  Type getElementType() const { return elementType; };

  /// Return the raw attribute component.
  Attribute getAttribute() const { return attr; };

private:
  ShapeStorageT dims;
  bool ranked;
  Type elementType;
  Attribute attr;
};

#include "mlir/Analysis/InferTypeOpInterface.h.inc"

namespace detail {
// Helper function to infer return tensor returns types given element and shape
// inference function.
//
// TODO: Consider generating typedefs for trait member functions if this usage
// becomes more common.
LogicalResult inferReturnTensorTypes(
    function_ref<LogicalResult(
        MLIRContext *, Optional<Location> location, ValueRange operands,
        ArrayRef<NamedAttribute> attributes, RegionRange regions,
        SmallVectorImpl<ShapedTypeComponents> &retComponents)>
        componentTypeFn,
    MLIRContext *context, Optional<Location> location, ValueRange operands,
    ArrayRef<NamedAttribute> attributes, RegionRange regions,
    SmallVectorImpl<Type> &inferedReturnTypes);
} // namespace detail

namespace OpTrait {

/// Tensor type inference trait that constructs a tensor from the infered
/// shape and elemental types.
/// Requires: Op implements functions of InferShapedTypeOpInterface.
template <typename ConcreteType>
class InferTensorType : public TraitBase<ConcreteType, InferTensorType> {
public:
  static LogicalResult
  inferReturnTypes(MLIRContext *context, Optional<Location> location,
                   ValueRange operands, ArrayRef<NamedAttribute> attributes,
                   RegionRange regions,
                   SmallVectorImpl<Type> &inferedReturnTypes) {
    return ::mlir::detail::inferReturnTensorTypes(
        ConcreteType::inferReturnTypeComponents, context, location, operands,
        attributes, regions, inferedReturnTypes);
  }
};

} // namespace OpTrait
} // namespace mlir

#endif // MLIR_ANALYSIS_INFERTYPEOPINTERFACE_H_
