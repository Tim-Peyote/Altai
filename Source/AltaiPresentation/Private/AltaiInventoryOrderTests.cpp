#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AltaiInventoryOrder.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAltaiInventoryOrderTest,"Altai.Inventory.ViewSorting",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAltaiInventoryOrderTest::RunTest(const FString&)
{
 TArray<FAltaiItem> Items;
 for(int32 N=0;N<4;++N){FAltaiItem I;I.Id=FName(*FString::Printf(TEXT("id%d"),N));I.Name=FText::FromString(N%2?TEXT("Alpha"):TEXT("Zulu"));I.Quantity=N+1;I.Category=N==0?EAltaiItemCategory::Tool:EAltaiItemCategory::Material;Items.Add(I);}
 const auto Original=Items;TArray<int32> View={0,1,2,3};AltaiSortInventory(Items,View,0);
 TestEqual(TEXT("Category before alphabetical order"),View.Last(),0);
 AltaiSortInventory(Items,View,1);TestEqual(TEXT("Same names break ties by stable identity"),View[0],1);TestEqual(TEXT("Second equal name"),View[1],3);
 AltaiSortInventory(Items,View,2);TestEqual(TEXT("Largest quantity first"),View[0],3);
 View={2,0};AltaiSortInventory(Items,View,2);TestEqual(TEXT("Sorting a filtered view preserves membership"),View.Num(),2);TestEqual(TEXT("Filtered first item"),View[0],2);
 for(int32 N=0;N<Items.Num();++N)TestEqual(TEXT("Owned array remains in original order"),Items[N].Id,Original[N].Id);
 return true;
}
#endif
