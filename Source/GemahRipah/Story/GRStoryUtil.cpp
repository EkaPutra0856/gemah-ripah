#include "Story/GRStoryUtil.h"
#include "Components/MeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

FLinearColor GRStoryUtil::ParseColor(const FString& Hex)
{
	const FString Clean = Hex.Replace(TEXT("#"), TEXT(""));
	if (Clean.Len() < 6)
	{
		return FLinearColor(0.9f, 0.6f, 0.2f);
	}
	return FLinearColor(FColor::FromHex(Clean));
}

UStaticMesh* GRStoryUtil::LoadShape(const FString& Shape)
{
	const TCHAR* Path = TEXT("/Engine/BasicShapes/Cube.Cube");
	if (Shape == TEXT("cylinder"))
	{
		Path = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	}
	else if (Shape == TEXT("sphere"))
	{
		Path = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	}
	else if (Shape == TEXT("cone"))
	{
		Path = TEXT("/Engine/BasicShapes/Cone.Cone");
	}
	return LoadObject<UStaticMesh>(nullptr, Path);
}

UMaterialInstanceDynamic* GRStoryUtil::MakeColorMaterial(UObject* Outer, const FLinearColor& Color, float LineFactor)
{
	static TWeakObjectPtr<UMaterialInterface> Parent;
	if (!Parent.IsValid())
	{
		Parent = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/LevelPrototyping/Materials/M_PrototypeGrid.M_PrototypeGrid"));
	}
	if (!Parent.IsValid())
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Parent.Get(), Outer);
	const FLinearColor Line = Color * LineFactor;
	Mid->SetVectorParameterValue(TEXT("SurfaceColor"), Color);
	Mid->SetVectorParameterValue(TEXT("TopSurfaceColor"), Color);
	Mid->SetVectorParameterValue(TEXT("GridColor"), Line);
	Mid->SetVectorParameterValue(TEXT("TopGridColor"), Line);
	Mid->SetVectorParameterValue(TEXT("SubGridColor"), Line);
	Mid->SetVectorParameterValue(TEXT("TopSubGridGridColor"), Line);
	return Mid;
}

void GRStoryUtil::ApplyColor(UMeshComponent* Mesh, const FLinearColor& Color)
{
	UMaterialInstanceDynamic* Mid = Mesh ? MakeColorMaterial(Mesh, Color) : nullptr;
	if (!Mid)
	{
		return;
	}
	for (int32 i = 0; i < FMath::Max(1, Mesh->GetNumMaterials()); ++i)
	{
		Mesh->SetMaterial(i, Mid);
	}
}

FVector GRStoryUtil::ToLocation(const TArray<float>& Pos, float DefaultZ)
{
	return FVector(Pos.IsValidIndex(0) ? Pos[0] : 0.f, Pos.IsValidIndex(1) ? Pos[1] : 0.f, Pos.IsValidIndex(2) ? Pos[2] : DefaultZ);
}
