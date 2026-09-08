#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AltaiExpedition.h"
#include "Engine/GameInstance.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAltaiInventoryTest,"Altai.Inventory.StacksAndGrowth",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAltaiInventoryTest::RunTest(const FString& Parameters)
{
 auto* S=NewObject<UAltaiExpedition>(NewObject<UGameInstance>());
 FAltaiItem Item;Item.Id=TEXT("Stone");Item.Quantity=3;
 TestTrue(TEXT("Add first stack"),S->AddItem(Item));
 TestTrue(TEXT("Merge stack"),S->AddItem(Item));
 TestEqual(TEXT("One cell after merge"),S->GetItems().Num(),1);
 TestEqual(TEXT("Merged quantity"),S->GetItems()[0].Quantity,6);
 Item.Quantity=0;TestFalse(TEXT("Reject zero quantity"),S->AddItem(Item));
 Item.Quantity=-1;TestFalse(TEXT("Reject negative quantity"),S->AddItem(Item));
 Item.Quantity=1;Item.Id=NAME_None;TestFalse(TEXT("Reject missing ID"),S->AddItem(Item));
 for(int32 I=1;I<100;++I){Item.Id=FName(*FString::Printf(TEXT("Item%d"),I));TestTrue(TEXT("Grow inventory past visible rows"),S->AddItem(Item));}
 TestEqual(TEXT("All 100 stacks retained"),S->GetItems().Num(),100);
 Item.Id=TEXT("Stone");TestTrue(TEXT("Existing stack still merges in large inventory"),S->AddItem(Item));
 Item.Quantity=MAX_int32;TestFalse(TEXT("Reject integer overflow"),S->AddItem(Item));
 TestFalse(TEXT("Reject invalid removal"),S->RemoveOne(-1));
 TestTrue(TEXT("Remove last unit of a stack"),S->RemoveOne(99));
 TestEqual(TEXT("Empty cell released"),S->GetItems().Num(),99);
 return true;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAltaiLoadoutTest,"Altai.Inventory.EquipmentOwnership",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAltaiLoadoutTest::RunTest(const FString& Parameters)
{
 auto* S=NewObject<UAltaiExpedition>(NewObject<UGameInstance>());
 FAltaiItem Coat;Coat.Id=TEXT("TestCoat");Coat.Category=EAltaiItemCategory::Equipment;Coat.WearSlot=EAltaiEquipSlot::Body;
 FAltaiItem Flask;Flask.Id=TEXT("TestFlask");Flask.Category=EAltaiItemCategory::Tool;Flask.QuickAccess=true;Flask.Quantity=2;
 TestFalse(TEXT("cannot equip unowned item"),S->AssignItem(Coat.Id,EAltaiEquipSlot::Body));
 S->AddItem(Coat);S->AddItem(Flask);
 TestFalse(TEXT("reject incompatible slot"),S->AssignItem(Coat.Id,EAltaiEquipSlot::Head));
 TestTrue(TEXT("equip owned coat"),S->AssignItem(Coat.Id,EAltaiEquipSlot::Body));
 TestTrue(TEXT("assign quick slot"),S->AssignItem(Flask.Id,EAltaiEquipSlot::Quick1));
 TestTrue(TEXT("move quick assignment"),S->AssignItem(Flask.Id,EAltaiEquipSlot::Quick2));
 TestEqual(TEXT("one assignment per item"),S->GetLoadout().Num(),2);
 TestEqual(TEXT("assignment does not consume item"),S->GetItems()[1].Quantity,2);
 S->RemoveOne(1);TestEqual(TEXT("remaining stack retains assignment"),S->GetLoadout().Num(),2);
 S->RemoveOne(1);TestEqual(TEXT("last unit clears assignment"),S->GetLoadout().Num(),1);
 TestTrue(TEXT("unequip"),S->ClearSlot(EAltaiEquipSlot::Body));
 TestEqual(TEXT("unequip keeps owned item"),S->GetItems().Num(),1);
 return true;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAltaiSlotMoveTest,"Altai.Inventory.AtomicSlotMoves",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAltaiSlotMoveTest::RunTest(const FString&)
{
 auto* S=NewObject<UAltaiExpedition>(NewObject<UGameInstance>());
 FAltaiItem A;A.Id=TEXT("A");A.QuickAccess=true;A.Quantity=3;FAltaiItem B=A;B.Id=TEXT("B");S->AddItem(A);S->AddItem(B);
 S->AssignItem(A.Id,EAltaiEquipSlot::Quick1);S->AssignItem(B.Id,EAltaiEquipSlot::Quick2);
 TestTrue(TEXT("Swap occupied compatible slots"),S->MoveAssignment(A.Id,EAltaiEquipSlot::Quick1,EAltaiEquipSlot::Quick2));
 auto Get=[&](EAltaiEquipSlot Slot)->FName{for(const auto& E:S->GetLoadout())if(E.Slot==Slot)return E.ItemId;return NAME_None;};
 TestEqual(TEXT("A moved"),Get(EAltaiEquipSlot::Quick2),A.Id);TestEqual(TEXT("B swapped to source"),Get(EAltaiEquipSlot::Quick1),B.Id);
 TestFalse(TEXT("Stale drag is rejected"),S->MoveAssignment(A.Id,EAltaiEquipSlot::Quick1,EAltaiEquipSlot::None));
 TestFalse(TEXT("Incompatible drop rejected"),S->MoveAssignment(A.Id,EAltaiEquipSlot::Quick2,EAltaiEquipSlot::Head));
 TestEqual(TEXT("Rejected drag keeps source"),Get(EAltaiEquipSlot::Quick2),A.Id);
 TestTrue(TEXT("Same-slot drop succeeds without move"),S->MoveAssignment(A.Id,EAltaiEquipSlot::Quick2,EAltaiEquipSlot::Quick2));
 TestTrue(TEXT("Drop into bag clears assignment"),S->MoveAssignment(A.Id,EAltaiEquipSlot::Quick2,EAltaiEquipSlot::None));
 TestEqual(TEXT("No stack consumed"),S->GetItems()[0].Quantity,3);TestEqual(TEXT("Other slot unchanged"),Get(EAltaiEquipSlot::Quick1),B.Id);
 S->AssignItem(A.Id,EAltaiEquipSlot::Quick1);TestEqual(TEXT("Bag assignment replaces occupied slot"),Get(EAltaiEquipSlot::Quick1),A.Id);TestEqual(TEXT("Replaced item remains owned"),S->GetItems().Num(),2);
 return true;
}
#endif
