#pragma once
#include "AltaiSession.h"

// Sort a view of the bag; never reorder ownership data or identify a drag by a cell index.
inline void AltaiSortInventory(const TArray<FAltaiItem>& Items,TArray<int32>& Indices,int32 Mode)
{
 Indices.StableSort([&](int32 A,int32 B)
 {
  const auto& X=Items[A];const auto& Y=Items[B];
  if(Mode==0 && X.Category!=Y.Category)return X.Category<Y.Category;
  if(Mode==2 && X.Quantity!=Y.Quantity)return X.Quantity>Y.Quantity;
  const int32 NameOrder=X.Name.CompareTo(Y.Name);
  return NameOrder==0?X.Id.LexicalLess(Y.Id):NameOrder<0;
 });
}
