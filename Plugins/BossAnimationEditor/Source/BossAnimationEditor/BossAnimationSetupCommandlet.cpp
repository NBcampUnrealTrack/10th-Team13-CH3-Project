#include "BossAnimationSetupCommandlet.h"
#include "Modules/ModuleManager.h"
#include "BossAnimInstance.h"
#include "BossCharacterBase.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimBlueprintGeneratedClass.h"
#include "Engine/SkeletalMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "Factories/AnimBlueprintFactory.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/CompilerResultsLog.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_StateMachine.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "AnimGraphNode_BlendListByInt.h"
#include "AnimGraphNode_StateResult.h"
#include "AnimGraphNode_TransitionResult.h"
#include "AnimationStateMachineGraph.h"
#include "AnimationStateGraph.h"
#include "AnimationTransitionGraph.h"
#include "AnimStateNode.h"
#include "AnimStateEntryNode.h"
#include "AnimStateTransitionNode.h"
#include "K2Node_VariableGet.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphSchema.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "UObject/UnrealType.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BossStatComponent.h"
#include "PlayerHealthComponent.h"
#include "Animation/AnimClassInterface.h"
#include "Animation/AnimNode_SequencePlayer.h"
#include "GameFramework/WorldSettings.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, BossAnimationEditor)

UBossAnimationSetupCommandlet::UBossAnimationSetupCommandlet()
{
    IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true;
}

namespace BossGraph
{
template<class T> T* Add(UEdGraph* Graph, int32 X, int32 Y)
{
    T* Node = NewObject<T>(Graph);
    Graph->AddNode(Node, false, false);
    Node->CreateNewGuid();
    Node->PostPlacedNewNode();
    if (Node->Pins.IsEmpty()) Node->AllocateDefaultPins();
    Node->NodePosX = X; Node->NodePosY = Y;
    return Node;
}
void Connect(UEdGraphPin* From, UEdGraphPin* To)
{
    check(From && To);
    check(From->GetOwningNode()->GetGraph()->GetSchema()->TryCreateConnection(From, To));
}
UEdGraphPin* Pose(UEdGraphNode* Node)
{
    for (UEdGraphPin* Pin : Node->Pins) if (Pin->Direction == EGPD_Output) return Pin;
    return nullptr;
}
UK2Node_VariableGet* Get(UEdGraph* Graph, FName Name, int32 X, int32 Y)
{
    auto* Node = NewObject<UK2Node_VariableGet>(Graph);
    Node->VariableReference.SetSelfMember(Name);
    Graph->AddNode(Node, false, false); Node->CreateNewGuid(); Node->AllocateDefaultPins();
    Node->NodePosX = X; Node->NodePosY = Y;
    return Node;
}
UAnimGraphNode_SequencePlayer* Sequence(UEdGraph* Graph, const TCHAR* Name, bool Loop, int32 X, int32 Y, float Rate = 1.f)
{
    FString Path = FString::Printf(TEXT("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Animations/%s.%s"), Name, Name);
    UAnimSequence* Asset = LoadObject<UAnimSequence>(nullptr, *Path); check(Asset);
    auto* Node = Add<UAnimGraphNode_SequencePlayer>(Graph, X, Y);
    Node->Node.SetSequence(Asset); Node->Node.SetLoopAnimation(Loop); Node->Node.SetPlayRate(Rate);
    Node->ReconstructNode();
    return Node;
}
void Transition(UAnimStateNode* From, UAnimStateNode* To, FName Condition)
{
    auto* Node = Add<UAnimStateTransitionNode>(From->GetGraph(), (From->NodePosX+To->NodePosX)/2, 0);
    Node->CreateConnections(From, To); Node->CrossfadeDuration = 0.2f;
    auto* Rule = CastChecked<UAnimationTransitionGraph>(Node->BoundGraph);
    auto* Value = Get(Rule, Condition, -250, 0);
    Connect(Value->GetValuePin(), Rule->GetResultNode()->FindPinChecked(TEXT("bCanEnterTransition")));
}
bool Save(UObject* Object)
{
    UPackage* Package = Object->GetOutermost();
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
    return UPackage::SavePackage(Package, Object, *FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension()), Args);
}
}

int32 UBossAnimationSetupCommandlet::Main(const FString& Params)
{
    using namespace BossGraph;
    if (Params.Contains(TEXT("Verify")))
    {
        auto* BP = LoadObject<UAnimBlueprint>(nullptr, TEXT("/Game/Boss/Animations/ABP_Boss.ABP_Boss")); check(BP);
        auto* Class = LoadClass<ABossCharacterBase>(nullptr, TEXT("/Game/Boss/Characters/BP_BossCharacterBase.BP_BossCharacterBase_C")); check(Class);
        auto* Defaults = CastChecked<ABossCharacterBase>(Class->GetDefaultObject());
        check(Defaults->GetMesh()->AnimClass == BP->GeneratedClass);
        check(Defaults->GetMesh()->GetAnimationMode() == EAnimationMode::AnimationBlueprint);
        FCompilerResultsLog Results;
        FKismetEditorUtilities::CompileBlueprint(BP, EBlueprintCompileOptions::None, &Results);
        check(Results.NumErrors == 0);
        const auto Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
        UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("BossAnimationTest"), nullptr, true, ERHIFeatureLevel::SM5, &Init);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        FActorSpawnParameters Spawn; Spawn.bDeferConstruction = true; Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* Boss = World->SpawnActor<ABossCharacterBase>(Class, FVector(0,0,1000), FRotator::ZeroRotator, Spawn);
        Boss->AutoPossessAI = EAutoPossessAI::Disabled;
        Boss->FinishSpawning(Boss->GetActorTransform());
        World->InitializeActorsForPlay(FURL());
        World->GetWorldSettings()->NotifyBeginPlay();
        World->GetWorldSettings()->NotifyMatchStarted();
        Boss->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
        Boss->GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        auto Advance = [&](float Seconds)
        {
            for (int32 I=0; I<FMath::CeilToInt(Seconds*60); ++I) { ++GFrameCounter; World->Tick(LEVELTICK_All, 1.f/60.f); }
        };
        auto Anim = [&]() { return CastChecked<UBossAnimInstance>(Boss->GetMesh()->GetAnimInstance()); };
        auto CheckMotion = [&](int32 Expected)
        {
            Advance(0.05f);
            checkf(Anim()->MotionIndex == Expected, TEXT("Expected %d, got %d; character=%d, begun=%d"), Expected, Anim()->MotionIndex, Boss->GetAnimationMotionIndex(), Boss->HasActorBegunPlay());
            UE_LOG(LogTemp, Display, TEXT("BOSS_VERIFY MotionIndex=%d passed"), Expected);
        };
        CheckMotion(0);
        Boss->GetCharacterMovement()->Velocity = FVector(300,0,0);
        Anim()->NativeUpdateAnimation(0.f); check(Anim()->bMoving && !Anim()->bIdle);
        Boss->GetCharacterMovement()->Velocity = FVector::ZeroVector;
        Anim()->NativeUpdateAnimation(0.f); check(!Anim()->bMoving && Anim()->bIdle);
        check(Boss->StartMagicAttack(FVector(1000,0,1000))); CheckMotion(1); Advance(4); CheckMotion(0);
        check(Boss->StartFallingRock(FVector(1000,0,0))); CheckMotion(2); Advance(4); CheckMotion(0);
        AActor* Target = World->SpawnActor<AActor>();
        auto* Health = NewObject<UPlayerHealthComponent>(Target); Target->AddInstanceComponent(Health); Health->RegisterComponent();
        check(Boss->StartVortex(Target)); CheckMotion(4); Advance(4); CheckMotion(0);
        Boss->UpdatePlayerNoDamageState(Target); Advance(21);
        check(Boss->StartBerserk(Target)); CheckMotion(3); Advance(21); CheckMotion(0);
        for (EBossPattern Pattern : { EBossPattern::GroundSmash, EBossPattern::CenterProjectile, EBossPattern::BlackHole })
        {
            Boss->PreparePatternAnimation(Pattern); check(Boss->GetMesh()->GetSingleNodeInstance());
            Boss->PreparePatternAnimation(EBossPattern::MagicAttack); check(Anim());
        }
        check(Boss->StartMagicAttack(FVector(1000,0,1000)));
        Boss->GetBossStatComponent()->ApplyDamage(100000.f); CheckMotion(5);
        Advance(12); check(Boss->IsHidden());
        Boss->CycleDebugHealthPhase(); CheckMotion(0); check(!Boss->IsHidden());
        World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
        UE_LOG(LogTemp, Display, TEXT("BOSS_VERIFY_SUCCESS: saved BP class, graph compile, Idle/Walk, all 5 motions, timed returns, legacy SingleNode handoff, death interruption and revive."));
        return 0;
    }
    const TCHAR* PackageName = TEXT("/Game/Boss/Animations/ABP_Boss");
    if (FPackageName::DoesPackageExist(PackageName))
    {
        UE_LOG(LogTemp, Error, TEXT("ABP_Boss already exists; refusing to overwrite.")); return 1;
    }
    USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Meshes/Sevarog.Sevarog"));
    check(Mesh);
    UAnimBlueprintFactory* Factory = NewObject<UAnimBlueprintFactory>();
    Factory->ParentClass = UBossAnimInstance::StaticClass(); Factory->TargetSkeleton = Mesh->GetSkeleton(); Factory->PreviewSkeletalMesh = Mesh;
    UAnimBlueprint* BP = CastChecked<UAnimBlueprint>(Factory->FactoryCreateNew(UAnimBlueprint::StaticClass(), CreatePackage(PackageName), TEXT("ABP_Boss"), RF_Public|RF_Standalone, nullptr, GWarn));
    UEdGraph* Graph = nullptr;
    for (UEdGraph* G : BP->FunctionGraphs) if (G->GetFName() == TEXT("AnimGraph")) Graph = G;
    check(Graph);
    TArray<UAnimGraphNode_Root*> Roots; Graph->GetNodesOfClass(Roots); check(Roots.Num()==1);
    auto* Machine = Add<UAnimGraphNode_StateMachine>(Graph, -700, -300);
    FBlueprintEditorUtils::RenameGraph(Machine->EditorStateMachineGraph, TEXT("Locomotion"));
    auto* Idle = Add<UAnimStateNode>(Machine->EditorStateMachineGraph, 100, 0);
    auto* Walk = Add<UAnimStateNode>(Machine->EditorStateMachineGraph, 450, 0);
    FBlueprintEditorUtils::RenameGraph(Idle->BoundGraph, TEXT("Idle"));
    FBlueprintEditorUtils::RenameGraph(Walk->BoundGraph, TEXT("Walk"));
    Connect(Pose(Sequence(Idle->BoundGraph, TEXT("Idle"), true, -350, 0)), Idle->GetPoseSinkPinInsideState());
    Connect(Pose(Sequence(Walk->BoundGraph, TEXT("Walk_Fwd"), true, -350, 0)), Walk->GetPoseSinkPinInsideState());
    TArray<UAnimStateEntryNode*> Entries; Machine->EditorStateMachineGraph->GetNodesOfClass(Entries); check(Entries.Num()==1);
    Connect(Entries[0]->Pins[0], Idle->GetInputPin());
    Transition(Idle, Walk, TEXT("bMoving")); Transition(Walk, Idle, TEXT("bIdle"));

    auto* Blend = Add<UAnimGraphNode_BlendListByInt>(Graph, -100, 0);
    // PostPlacedNewNode provides two poses. Add four attack/death inputs.
    for (int32 I=0; I<4; ++I) Blend->Node.AddPose();
    auto* Mode = FindFProperty<FEnumProperty>(FAnimNode_BlendListBase::StaticStruct(), TEXT("ChildUpateMode")); check(Mode);
    Mode->GetUnderlyingProperty()->SetIntPropertyValue(Mode->ContainerPtrToValuePtr<void>(&Blend->Node), static_cast<uint64>(EBlendListChildUpdateMode::ResetChildOnActivate));
    Blend->ReconstructNode();
    Connect(Pose(Machine), Blend->FindPinChecked(TEXT("BlendPose_0")));
    const TCHAR* Names[] = {TEXT("Cast"), TEXT("Ultimate_Swing_120fps"), TEXT("Knock_back_bwd"), TEXT("Cast"), TEXT("Death_front")};
    const TCHAR* Labels[] = {TEXT("MagicAttack"), TEXT("FallingRock"), TEXT("Berserk - Loop"), TEXT("Vortex"), TEXT("Death - Hold final pose")};
    for (int32 I=0; I<5; ++I)
    {
        auto* Player = Sequence(Graph, Names[I], I==2, -700, I*190, I==4 ? 0.4f : 1.f);
        Player->NodeComment = Labels[I]; Player->bCommentBubbleVisible = true;
        Connect(Pose(Player), Blend->FindPinChecked(*FString::Printf(TEXT("BlendPose_%d"), I+1)));
    }
    for (int32 I=0; I<6; ++I)
    {
        auto* Pin = Blend->FindPinChecked(*FString::Printf(TEXT("BlendTime_%d"), I));
        Graph->GetSchema()->TrySetDefaultValue(*Pin, I==5 ? TEXT("0.25") : TEXT("0.18"));
    }
    auto* Index = Get(Graph, TEXT("MotionIndex"), -430, -450);
    Connect(Index->GetValuePin(), Blend->FindPinChecked(TEXT("ActiveChildIndex")));
    Roots[0]->NodePosX = 350; Roots[0]->NodePosY = 0;
    Connect(Pose(Blend), Roots[0]->FindPinChecked(TEXT("Result")));
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FCompilerResultsLog Results;
    FKismetEditorUtilities::CompileBlueprint(BP, EBlueprintCompileOptions::None, &Results);
    if (Results.NumErrors > 0 || BP->Status == BS_Error) return 2;
    if (!Save(BP)) return 3;
    auto* BossBP = LoadObject<UBlueprint>(nullptr, TEXT("/Game/Boss/Characters/BP_BossCharacterBase.BP_BossCharacterBase")); check(BossBP);
    auto* Boss = CastChecked<ABossCharacterBase>(BossBP->GeneratedClass->GetDefaultObject());
    Boss->GetMesh()->SetAnimInstanceClass(BP->GeneratedClass);
    Boss->GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
    FBlueprintEditorUtils::MarkBlueprintAsModified(BossBP);
    FKismetEditorUtilities::CompileBlueprint(BossBP, EBlueprintCompileOptions::None, &Results);
    if (Results.NumErrors > 0 || !Save(BossBP)) return 4;
    UE_LOG(LogTemp, Display, TEXT("BOSS_ABP_SUCCESS: Locomotion Idle/Walk, 5 motion nodes, reset-on-activate, blends, BP mesh class saved."));
    return 0;
}
