#include "DAEParser.h"
#include "XmlParser.h"
#include "Misc/FileHelper.h"

// ─────────────────────────────────────────────────────────────────────────────
// Helper: parse a space-separated string of floats
// ─────────────────────────────────────────────────────────────────────────────
TArray<float> FDAEParser::ParseFloatArray(const FString& Text)
{
	TArray<float> Result;
	TArray<FString> Tokens;
	Text.ParseIntoArrayWS(Tokens);
	Result.Reserve(Tokens.Num());
	for (const FString& Token : Tokens)
	{
		Result.Add(FCString::Atof(*Token));
	}
	return Result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Helper: parse a space-separated string of ints
// ─────────────────────────────────────────────────────────────────────────────
TArray<int32> FDAEParser::ParseIntArray(const FString& Text)
{
	TArray<int32> Result;
	TArray<FString> Tokens;
	Text.ParseIntoArrayWS(Tokens);
	Result.Reserve(Tokens.Num());
	for (const FString& Token : Tokens)
	{
		Result.Add(FCString::Atoi(*Token));
	}
	return Result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Main parser
// ─────────────────────────────────────────────────────────────────────────────
bool FDAEParser::Parse(const FString& FilePath, FDAEMesh& OutMesh)
{
	FXmlFile XmlFile;
	if (!XmlFile.LoadFile(FilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("DAEParser: Failed to load %s"), *FilePath);
		return false;
	}

	const FXmlNode* Root = XmlFile.GetRootNode();
	if (!Root) return false;

	// Collect all geometry meshes — the KMZ may have more than one per lot
	const FXmlNode* LibGeo = Root->FindChildNode(TEXT("library_geometries"));
	if (!LibGeo) return false;

	// We accumulate across all geometry nodes in the file
	int32 VertexOffset = 0;

	for (const FXmlNode* GeoNode : LibGeo->GetChildrenNodes())
	{
		const FXmlNode* MeshNode = GeoNode->FindChildNode(TEXT("mesh"));
		if (!MeshNode) continue;

		// ── 1. Find position source ──────────────────────────────────────────
		// The vertices element tells us which source is POSITION
		const FXmlNode* VerticesNode = MeshNode->FindChildNode(TEXT("vertices"));
		FString PositionSourceId;
		if (VerticesNode)
		{
			for (const FXmlNode* Input : VerticesNode->GetChildrenNodes())
			{
				if (Input->GetAttribute(TEXT("semantic")) == TEXT("POSITION"))
				{
					PositionSourceId = Input->GetAttribute(TEXT("source"));
					// Strip leading '#'
					if (PositionSourceId.StartsWith(TEXT("#")))
					{
						PositionSourceId.RemoveAt(0);
					}
				}
			}
		}

		// ── 2. Find normal source ────────────────────────────────────────────
		FString NormalSourceId;
		// We'll grab it from the triangles inputs below

		// ── 3. Collect all sources into a map id->float array ────────────────
		TMap<FString, TArray<float>> Sources;
		for (const FXmlNode* SourceNode : MeshNode->GetChildrenNodes())
		{
			if (SourceNode->GetTag() != TEXT("source")) continue;
			FString SrcId = SourceNode->GetAttribute(TEXT("id"));
			const FXmlNode* FloatArrayNode = SourceNode->FindChildNode(TEXT("float_array"));
			if (FloatArrayNode)
			{
				Sources.Add(SrcId, ParseFloatArray(FloatArrayNode->GetContent()));
			}
		}

		// ── 4. Process each triangles element ────────────────────────────────
		for (const FXmlNode* TriNode : MeshNode->GetChildrenNodes())
		{
			if (TriNode->GetTag() != TEXT("triangles")) continue;

			// Determine input semantics and offsets
			int32 VertexOffset2   = 0;
			int32 NormalOffset2   = -1;
			int32 Stride          = 1; // number of indices per vertex in <p>

			for (const FXmlNode* Input : TriNode->GetChildrenNodes())
			{
				if (Input->GetTag() != TEXT("input")) continue;
				FString Semantic = Input->GetAttribute(TEXT("semantic"));
				int32   Offset   = FCString::Atoi(*Input->GetAttribute(TEXT("offset")));

				if (Semantic == TEXT("VERTEX"))
				{
					VertexOffset2 = Offset;
					Stride = FMath::Max(Stride, Offset + 1);
				}
				else if (Semantic == TEXT("NORMAL"))
				{
					NormalOffset2  = Offset;
					NormalSourceId = Input->GetAttribute(TEXT("source"));
					if (NormalSourceId.StartsWith(TEXT("#")))
						NormalSourceId.RemoveAt(0);
					Stride = FMath::Max(Stride, Offset + 1);
				}
			}

			// Fetch raw float data
			const TArray<float>* PosData = Sources.Find(PositionSourceId);
			const TArray<float>* NrmData = NormalSourceId.IsEmpty() ? nullptr : Sources.Find(NormalSourceId);

			if (!PosData || PosData->Num() == 0) continue;

			// Parse the <p> index list
			const FXmlNode* PNode = TriNode->FindChildNode(TEXT("p"));
			if (!PNode) continue;

			TArray<int32> PIndices = ParseIntArray(PNode->GetContent());

			int32 NumTriangles = PIndices.Num() / (Stride * 3);

			for (int32 t = 0; t < NumTriangles; ++t)
			{
				for (int32 v = 0; v < 3; ++v)
				{
					int32 Base = (t * 3 + v) * Stride;

					// Vertex
					int32 VI = PIndices[Base + VertexOffset2] * 3;
					if (VI + 2 < PosData->Num())
					{
						// DAE is Z-up; UE5 is Z-up too, but X/Y axes differ.
						// DAE X -> UE X, DAE Y -> UE Y, DAE Z -> UE Z
						FVector Pos(
							(*PosData)[VI],
							(*PosData)[VI + 1],
							(*PosData)[VI + 2]
						);
						OutMesh.Vertices.Add(Pos);
					}

					// Normal
					if (NrmData && NormalOffset2 >= 0)
					{
						int32 NI = PIndices[Base + NormalOffset2] * 3;
						if (NI + 2 < NrmData->Num())
						{
							FVector Nrm(
								(*NrmData)[NI],
								(*NrmData)[NI + 1],
								(*NrmData)[NI + 2]
							);
							OutMesh.Normals.Add(Nrm);
						}
						else
						{
							OutMesh.Normals.Add(FVector::UpVector);
						}
					}
					else
					{
						OutMesh.Normals.Add(FVector::UpVector);
					}

					// Triangle index — sequential since we're expanding
					OutMesh.Triangles.Add(VertexOffset + OutMesh.Vertices.Num() - 1);
				}
			}
		}

		VertexOffset = OutMesh.Vertices.Num();
	}

	return OutMesh.Vertices.Num() > 0;
}
