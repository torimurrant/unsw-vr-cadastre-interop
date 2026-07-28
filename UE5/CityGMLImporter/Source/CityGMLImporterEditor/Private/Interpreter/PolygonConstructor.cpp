#include "PolygonConstructor.h"


#include "AssetToolsModule.h"
#include "ProceduralMeshConversion.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "CompGeom/PolygonTriangulation.h"
#include "Dialogs/DlgPickAssetPath.h"
#include "Misc/GeneratedTypeName.h"
#include "MeshDescription.h"

#include "ProceduralMeshConversion.h"
#include "Modules/ModuleManager.h"
#include "Misc/PackageName.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Application/SlateWindowHelper.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Engine/StaticMesh.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "DetailCategoryBuilder.h"
#include "IDetailsView.h"
#include "ProceduralMeshComponent.h"
#include "StaticMeshAttributes.h"
#include "PhysicsEngine/BodySetup.h"
#include "Dialogs/DlgPickAssetPath.h"
#include "AssetRegistry/AssetRegistryModule.h"

#define LOCTEXT_NAMESPACE "PolygonConstructor"

PolygonConstructor::PolygonConstructor()
{
	
}
FVector PolygonConstructor::GenerateMesh(int& SectionNum, UProceduralMeshComponent* MeshProc, UStaticMeshComponent* Mesh)
{
	if (MeshProc)
	{
		MeshProc->CreateMeshSection(SectionNum, Vertices, Triangles, Normals, UVs, TArray<FColor>(), Tangents, true);
		Mesh->SetRelativeLocation(FVector(RelativeLocation));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MeshProcEmpty"));
	}
	
	
	return RelativeLocation;
}
	
FProcMeshTangent PolygonConstructor::ResetMesh()
{
	Vertices.Reset(); 
	TwoDVertices.Reset();
	Triangles.Reset();
	Normals.Reset();
	Tangents.Reset();
	UVs.Reset();
	Colors.Reset();
	isfirsttime=true;
	
	return FProcMeshTangent(0.f, 1.f, 0.f);;
}

void PolygonConstructor::AddPolygon(TArray<FVector> RawVertices, int32& TriIndex)
{
	
	try
	{
		FVector ThisNorm;
		int RawSize = RawVertices.Num();
		if(isfirsttime)
		{
			RawVertices.Pop();
			RawSize = RawVertices.Num();
			for (int i = 0; i < RawSize; ++i)
			{
				RawVertices[i].operator-=(RegulationVector);
			}
			RelativeLocation = RawVertices[0];
			RelativeLocation.operator*=(-1);
			isfirsttime=false;
		}
		if(RawVertices.Num()<3||RawVertices.IsEmpty())return;
		TArray<FVector>edges;
		for (int i = 0; i < RawSize; ++i)
		{
			if (RawVertices[i%RawSize].Equals(RawVertices[(i+1)%RawSize],tolerance))
			{
				RawVertices.RemoveAt(i);
				RawSize--;
				i--;
			}
		}
		for (int i = 0; i < RawSize; ++i)
		{
			edges.Add(RawVertices[(i+1)%RawVertices.Num()].operator-(RawVertices[i]));
		}
		
		TArray<FVector>Normalizededges = edges;
		for (int i=0; i<RawSize;i++)
		{
			Normalizededges[i].Normalize();
			if(abs(Normalizededges[i].X)==0) Normalizededges[i].X = 0;
			if(abs(Normalizededges[i].Y)==0) Normalizededges[i].Y = 0;
			if(abs(Normalizededges[i].Z)==0) Normalizededges[i].Z = 0;
		}
		int iterator=0;
		while (iterator < Normalizededges.Num())
		{
			if (RawSize<3||RawVertices.IsEmpty())return;
			if (Normalizededges[iterator].Equals(Normalizededges[(iterator+1)%Normalizededges.Num()],tolerance))
			{
				RawVertices.RemoveAt((iterator+1)%Normalizededges.Num());
				RawSize--;
				edges.RemoveAt((iterator+1)%Normalizededges.Num());
				edges[iterator%RawSize]=RawVertices[(iterator+1)%RawSize].operator-(RawVertices[iterator%RawSize]);
				Normalizededges.RemoveAt((iterator+1)%Normalizededges.Num());
			}
			else
			{
				iterator++;
			}
		}
		for (int i = 0; i < RawVertices.Num(); ++i)
		{
			FVector NegativeNormalizedEdge = -Normalizededges[i];
			if(abs(NegativeNormalizedEdge.X)<tolerance) NegativeNormalizedEdge.X = 0;
			if(abs(NegativeNormalizedEdge.Y)<tolerance) NegativeNormalizedEdge.Y = 0;
			if(abs(NegativeNormalizedEdge.Z)<tolerance) NegativeNormalizedEdge.Z = 0;
			for (int j = i+1; j%RawVertices.Num()!=i; ++j)
			{
				bool crossing = DoesIntersect(RawVertices[j%RawVertices.Num()],edges[j%RawVertices.Num()],Normalizededges[j%RawVertices.Num()],RawVertices[i],edges[i],Normalizededges[i],DBL_MAX); //Geht besser als O(n^2)
				if (crossing)
				{
					if (Normalizededges[j%RawVertices.Num()].Equals(Normalizededges[i],0.001)||Normalizededges[j%RawVertices.Num()].Equals(NegativeNormalizedEdge,0.001))
					{
						TArray<FVector> NewVerticies;
						for (int s = i+1; i != s%RawVertices.Num(); ++s)
						{
							if (s<=i||s>=j+1)
							{
								NewVerticies.Add(RawVertices[s%RawVertices.Num()]);
							}
						}
						NewVerticies.Add(RawVertices[i]);
						AddPolygon(NewVerticies,TriIndex);
						NewVerticies.Empty();
						for (int s = i+1; i != s%RawVertices.Num(); ++s)
						{
							if (!(s<=i||s>=j+1))
							{
								NewVerticies.Add(RawVertices[s%RawVertices.Num()]);
							}
						}
						AddPolygon(NewVerticies,TriIndex);
						return;
					}

					FVector PointonLine = RawVertices[i].operator+(0.5 *LineIntersection.operator-(RawVertices[i]));
					bool outsidelinei = PointInside(RawVertices,edges,Normalizededges,PointonLine);
					PointonLine = RawVertices[j%RawVertices.Num()].operator+(0.5 *LineIntersection.operator-(RawVertices[j%RawVertices.Num()]));
					bool outsidelinej = PointInside(RawVertices,edges,Normalizededges,PointonLine);
					PointonLine = LineIntersection.operator+(0.5*RawVertices[(i+1)%RawVertices.Num()].operator-(LineIntersection));
					bool outsidelineii = PointInside(RawVertices,edges,Normalizededges,PointonLine);
					PointonLine = LineIntersection.operator+(0.5*RawVertices[(j+1)%RawVertices.Num()].operator-(LineIntersection));
					bool outsidelinejj = PointInside(RawVertices,edges,Normalizededges,PointonLine);
					if (!(outsidelinei&&outsidelineii&&outsidelinej&&outsidelinejj))
					{
						TArray<FVector> NewVerticies;
						for (int s = i+1; i != s%RawVertices.Num(); ++s)
						{
							if (s<=i||s>=j+1)
							{
								NewVerticies.Add(RawVertices[s%RawVertices.Num()]);
							}
						}
						NewVerticies.Add(RawVertices[i]);
						NewVerticies.Add(LineIntersection);
						AddPolygon(NewVerticies,TriIndex);
						NewVerticies.Empty();
						for (int s = i+1; i != s%RawVertices.Num(); ++s)
						{
							if (!(s<=i||s>=j+1))
							{
								NewVerticies.Add(RawVertices[s%RawVertices.Num()]);
								if(s==j)NewVerticies.Add(LineIntersection);
							}
						}
						AddPolygon(NewVerticies,TriIndex);
						return;
					}
				}
			}
		}
		if(RawVertices.Num()<3||RawVertices.IsEmpty())return;
		FVector currentVertex = RawVertices[0];
		FVector nextVertex = RawVertices[1];
		int Vertexnum=0;
		TArray<FVector> TempVertices;
		bool gotIntersections;
		do
		{
			FVector Intersections;
			int Intersectionnum = 0;
			bool intersects =false;
			TempVertices.Add(currentVertex);
			double Intersectionlength = DBL_MAX;
			gotIntersections=false;
			for (int i = 0; i < RawVertices.Num(); ++i)
			{
				if(RawVertices[i]!=currentVertex)
				{
					FVector nextLine = (nextVertex.operator-(currentVertex));
					FVector normalizednextLine = nextLine;
					normalizednextLine.Normalize();
					intersects = DoesIntersect(RawVertices[i],edges[i],Normalizededges[i],currentVertex,nextLine,normalizednextLine,0.999);
					if (intersects&&LineIntersection.Length()<Intersectionlength&&!LineIntersection.Equals(currentVertex))
					{
						gotIntersections= true;
						Intersections = LineIntersection;
						Intersectionlength= LineIntersection.Length();
						Intersectionnum = i;
					}
				}
			}
			if (gotIntersections)
			{
				TempVertices.Add(Intersections);
				currentVertex = Intersections;
				nextVertex = RawVertices[(Intersectionnum)%RawVertices.Num()];
				Vertexnum = (Intersectionnum)%RawVertices.Num();
			}
			else
			{
				TempVertices.Add(nextVertex);
				currentVertex = nextVertex;
				nextVertex = RawVertices[(Vertexnum++)%RawVertices.Num()];
			}
		}
		while (currentVertex!= RawVertices[0]);
		if(gotIntersections)
		{
			TempVertices.Pop();
			RawVertices=TempVertices;
			RawSize = RawVertices.Num();
			for (int i = 0; i < RawSize; ++i)
			{
				if (RawVertices[i%RawSize].Equals(RawVertices[(i+1)%RawSize],tolerance))
				{
					RawVertices.RemoveAt(i);
					RawSize--;
					i--;
				}
			}
			edges.Empty();
			for (int i = 0; i < RawSize; ++i)
			{
				edges.Add(RawVertices[(i+1)%RawVertices.Num()].operator-(RawVertices[i]));
			}

			Normalizededges = edges;
			for (int i=0; i<RawSize;i++)
			{
				Normalizededges[i].Normalize();
				if(abs(Normalizededges[i].X)<tolerance) Normalizededges[i].X = 0;
				if(abs(Normalizededges[i].Y)<tolerance) Normalizededges[i].Y = 0;
				if(abs(Normalizededges[i].Z)<tolerance) Normalizededges[i].Z = 0;
			}
			iterator=0;
			while (iterator < Normalizededges.Num())
			{
				if (RawSize<3)return;
				if (Normalizededges[iterator].Equals(Normalizededges[(iterator+1)%Normalizededges.Num()],tolerance))
				{
					RawVertices.RemoveAt((iterator+1)%Normalizededges.Num());
					RawSize--;
					edges.RemoveAt((iterator+1)%Normalizededges.Num());
					edges[iterator%RawSize]=RawVertices[(iterator+1)%RawSize].operator-(RawVertices[iterator%RawSize]);
					Normalizededges.RemoveAt((iterator+1)%Normalizededges.Num());
				}
				else
				{
					iterator++;
				}
			}
		}
		TArray<int32> Points;
		for (int i = 0; i < RawSize; i++)
		{
			Points.Add(TriIndex++);
			Vertices.Add((RawVertices[i]));
		}
		FVector NewLine;
		FVector NormalizedNewLine;
		FVector PointOnNewLine;
		int Current = RawVertices.Num();
		int Suspect;
		bool onePlane = false;
		bool isNewLineInside = false;
		int CPoint = 0;
		int CSPoint = 0;
		int C2SPoint = 0;
		if (RawVertices.Num() == 3) goto Finish;
		if (RawVertices.Num() < 3 || RawVertices.IsEmpty()) return;
		onePlane = IsOnePlane(RawVertices);
		ThisNorm = -FVector::CrossProduct(RawVertices[RawSize - 1] - RawVertices[0], RawVertices[RawSize - 1] - RawVertices[2]).GetSafeNormal();
		Verticesto2D(RawVertices);
	Main:
		if (isBack)
		{
			Suspect = -1;
		}
		Suspect = 1;
		NewLine = RawVertices[(Current + 2) % RawVertices.Num()].operator-(RawVertices[Current % RawVertices.Num()]);
		NormalizedNewLine = NewLine;
		NormalizedNewLine.Normalize();
		goto Intersection;

		NewLine = RawVertices[(Current - 2) % RawVertices.Num()].operator-(RawVertices[Current % RawVertices.Num()]);
		NormalizedNewLine = NewLine;

	Intersection:
		NormalizedNewLine = NewLine;
		NormalizedNewLine.Normalize();
		if (abs(NormalizedNewLine.X) < tolerance) NormalizedNewLine.X = 0;
		if (abs(NormalizedNewLine.Y) < tolerance) NormalizedNewLine.Y = 0;
		if (abs(NormalizedNewLine.Z) < tolerance) NormalizedNewLine.Z = 0;
		for (int i = 0; i < Normalizededges.Num(); ++i)
		{
			bool hasIntersection = DoesIntersect(RawVertices[i], edges[i], Normalizededges[i], RawVertices[Current % RawVertices.Num()], NewLine, NormalizedNewLine, 1.0);
			if (Current > 15 * RawSize)
			{
				breaks++;
				onePlane = IsOnePlane(RawVertices);
				goto Finish;
			}
			if (hasIntersection)
			{
				if (!isBack)
				{
					Current++;
					isBack = false;
				}
				goto Main;
			}
		}

		PointOnNewLine = RawVertices[Current % RawVertices.Num()].operator+(NewLine / 2.0);
		isNewLineInside = PointInside(RawVertices, edges, Normalizededges, PointOnNewLine);
		if (Current > 15 * RawSize)
		{
			breaks++;
			onePlane = IsOnePlane(RawVertices);
			goto Finish;
		}
		if (!isNewLineInside)
		{
			if (!isBack)
			{
				Current++;
				isBack = false;
			}
			goto Main;
		}

		CPoint = Current % RawVertices.Num();
		CSPoint = (Current + Suspect) % RawVertices.Num();
		C2SPoint = (Current + 2 * Suspect) % RawVertices.Num();
		Triangles.Add(Points[CPoint]);
		Triangles.Add(Points[CSPoint]);
		Triangles.Add(Points[C2SPoint]);
		RawVertices.RemoveAt(CSPoint);
		Points.RemoveAt(CSPoint);
		edges.RemoveAt(CPoint);
		edges.RemoveAt(CPoint % edges.Num());
		if (CPoint % (edges.Num() + 1) == 0 && CPoint != 0) edges.Add(NewLine);
		else edges.Insert(NewLine, CPoint % (edges.Num() + 1));
		Normalizededges.RemoveAt(CPoint);
		Normalizededges.RemoveAt(CPoint % Normalizededges.Num());
		if (CPoint % (Normalizededges.Num() + 1) == 0 && CPoint != 0) Normalizededges.Add(NormalizedNewLine);
		else Normalizededges.Insert(NormalizedNewLine, CPoint % (Normalizededges.Num() + 1));
		if (RawVertices.Num() > 3)
		{
			goto Main;
		}

	Finish:
		Triangles.Add(Points[0]);
		Triangles.Add(Points[1]);
		Triangles.Add(Points[2]);
		FProcMeshTangent Tangenten = FProcMeshTangent();
		Tangenten.TangentX = ThisNorm;
		for (int i = 0; i < RawSize; i++)
		{
			Tangenten.TangentX = ThisNorm;
			Normals.Add(ThisNorm);
			Tangents.Add(Tangenten);
			Colors.Add(FLinearColor::Green);
		}
	}
	catch (std::exception e) {
		return;
	}
}

bool PolygonConstructor::DoesIntersect(FVector EdgeStart, FVector Edge,FVector NormalizedEdge, FVector NewLineStart, FVector NewLine, FVector NormalizedNewLine, double paraleluppervalue)
{ 
	FVector NegativeNormalizedEdge = -NormalizedEdge;
	if(abs(NegativeNormalizedEdge.X)==0) NegativeNormalizedEdge.X = 0;
	if(abs(NegativeNormalizedEdge.Y)==0) NegativeNormalizedEdge.Y = 0;
	if(abs(NegativeNormalizedEdge.Z)==0) NegativeNormalizedEdge.Z = 0;
	if(!(NormalizedNewLine.Equals(NormalizedEdge,tolerance)||NormalizedNewLine.Equals(NegativeNormalizedEdge,tolerance)))
	{		
		double y;
		double x1;

		FVector C = EdgeStart;
		FVector D = NewLineStart; 
		if (!C.Equals(D,tolerance))
		{
			double h = FVector::CrossProduct(NewLine, D.operator-(C)).GetAbsMax();
			double k = FVector::CrossProduct(NewLine,Edge).GetAbsMax();
			FVector j = h/k*Edge;
			FVector Intersection;
			FVector NormVec1 = (FVector::CrossProduct(NewLine, D.operator-(C)));
			NormVec1.Normalize();
			FVector NormVec2 = FVector::CrossProduct(NewLine,Edge);
			NormVec2.Normalize();
			if (NormVec1.Equals(NormVec2,0.1))
			{
				Intersection = C.operator+(j);
			}
			else
			{
				Intersection = C.operator-(j);
			}
			if(abs(NewLine.X)>tolerance )
			{
				x1 = (Intersection.X-D.X)/NewLine.X;
			}
			else
			{
				if (abs(NewLine.Y)>tolerance)
				{
					x1 = (Intersection.Y-D.Y)/NewLine.Y;
				}
				else
				{
					x1 = (Intersection.Z-D.Z)/NewLine.Z;
				}
			}
			if (abs(Edge.X)>tolerance)
			{
				y = (Intersection.X-C.X)/Edge.X;
			}
			else
			{
				if (abs(Edge.Y)>tolerance)
				{
					y = (Intersection.Y-C.Y)/Edge.Y;
				}
				else
				{
					y = (Intersection.Z-C.Z)/Edge.Z;
				}
			}
			if((0<x1&&x1<1)&&(0<y&&y<1))
			{
				LineIntersection = Intersection;
				return true;
			}
		}
	}
	else
	{
		double factori;
		if (abs(Edge.X)>tolerance)
		{
			factori = (NewLineStart.X-EdgeStart.X)/Edge.X;
		}
		else
		{
			if (abs(Edge.Y)>tolerance)
			{
				factori = (NewLineStart.Y-EdgeStart.Y)/Edge.Y;
			}
			else
			{
				factori = (NewLineStart.Z-EdgeStart.Z)/Edge.Z;
			}
		}
		if (EdgeStart+factori*Edge==NewLineStart&&factori>0&&factori<paraleluppervalue)
		{
			LineIntersection = NewLineStart;
			return true;
		}
	}
	return false;
}

bool PolygonConstructor::PointInside(TArray<FVector> RawVertices, TArray<FVector> Edges,TArray<FVector> NormalizedEdges,FVector PointonNewLine)
{
	int cuts = 0;
	FVector cutvector = NormalizedEdges[0];
		for (int i = 0; i < NormalizedEdges.Num(); i++)
		{
			FVector NegativeNormalizedEdge = -NormalizedEdges[i];
			if(abs(NegativeNormalizedEdge.X)<tolerance) NegativeNormalizedEdge.X = 0;
			if(abs(NegativeNormalizedEdge.Y)<tolerance) NegativeNormalizedEdge.Y = 0;
			if(abs(NegativeNormalizedEdge.Z)<tolerance) NegativeNormalizedEdge.Z = 0;
			if(!(cutvector.Equals(NormalizedEdges[i],tolerance)||cutvector.Equals(NegativeNormalizedEdge,tolerance)))
			{
				double x2;
				x2=0;
				double y2;
				FVector C2;
				C2.Set(RawVertices[i].X,RawVertices[i].Y,RawVertices[i].Z);
				FVector D2;
				double h;
				double k;
				D2.Set(PointonNewLine.X,PointonNewLine.Y,PointonNewLine.Z);
				if (!C2.Equals(D2,tolerance))
				{
					h = FVector::CrossProduct(cutvector, D2.operator-(C2)).GetAbsMax();
					k = FVector::CrossProduct(cutvector,Edges[i]).GetAbsMax();
					FVector j = h/k*Edges[i];
					FVector Intersection;
					FVector NormVec1 = (FVector::CrossProduct(cutvector, D2.operator-(C2)));
					//UE_LOG(LogTemp, Warning, TEXT("%s"), *NormVec1.ToString());
					NormVec1.Normalize();
					//UE_LOG(LogTemp, Warning, TEXT("%s"), *NormVec1.ToString());
					if(abs(NormVec1.X)<tolerance) NormVec1.X =0;
					if(abs(NormVec1.Y)<tolerance) NormVec1.Y =0;
					if(abs(NormVec1.Z)<tolerance) NormVec1.Z =0;
					FVector NormVec2 = FVector::CrossProduct(cutvector,Edges[i]);
					//UE_LOG(LogTemp, Warning, TEXT("%s"), *NormVec2.ToString());
					NormVec2.Normalize();
					if(abs(NormVec2.X)<tolerance) NormVec2.X=0;
					if(abs(NormVec2.Y)<tolerance) NormVec2.Y=0;
					if(abs(NormVec2.Z)<tolerance) NormVec2.Z=0;
					//UE_LOG(LogTemp, Warning, TEXT("%s"), *NormVec2.ToString());
					if (NormVec1.Equals(NormVec2,0.1))
					{
						Intersection = C2.operator+(j);
					}
					else
					{
						Intersection = C2.operator-(j);
					}
					if (abs(cutvector.X)>tolerance)
					{
						x2 = (Intersection.X-D2.X)/cutvector.X;
					}
					else
					{
						if (abs(cutvector.Y)>tolerance)
						{
							x2 = (Intersection.Y-D2.Y)/cutvector.Y;
						}
						else
						{
							x2 = (Intersection.Z-D2.Z)/cutvector.Z;
						}
					}
					if (abs(Edges[i].X)>tolerance)
					{
						y2 = (Intersection.X-C2.X)/Edges[i].X;
					}
					else
					{
						if (abs(Edges[i].Y)>tolerance)
						{
							y2 = (Intersection.Y-C2.Y)/Edges[i].Y;
						}
						else
						{
							y2 = (Intersection.Z-C2.Z)/Edges[i].Z;
						}
					}
					if (0<y2&&y2<=1&&x2>0)
					{
						FString a = C2.ToString();
						FString b = D2.ToString();
						FString c = NormalizedEdges[i].ToString();
						FString d = cutvector.ToString();
						FString e = (D2.operator-(C2)).ToString();
						FString f = Intersection.ToString();
						FString g;
						if (isBack)
						{
							g = "Back";
						}
						else
						{
							g="Front";
						}
						//UE_LOG(LogTemp, Warning, TEXT("Outside Check intersect C:%s D:%s E: %s F: %s G: %s H: %f K: %f x : %f y: %f Intersection : %s Side:%b"),*a,*b,*c,*d,*e,h,k,x2,y2,*f,*g);
						cuts++;
					}
					else
					{
						FString a = C2.ToString();
						FString b = D2.ToString();
						FString c = NormalizedEdges[i].ToString();
						FString d = cutvector.ToString();
						FString e = (D2.operator-(C2)).ToString();
						FString f = Intersection.ToString();
						FString g;
						if (isBack)
						{
							g = "Back";
						}
						else
						{
							g="Front";
						}
						
						//UE_LOG(LogTemp, Warning, TEXT("Outside Check passes C:%s D:%s E: %s F: %s G: %s H: %f K: %f x : %f y: %f Intersection : %s Side:%s"),*a,*b,*c,*d,*e,h,k,x2,y2,*f,*g);
					}
				}
			}
			else 
			{
				double factor;
				if (abs(Edges[i].X)>tolerance)
				{
					factor = (RawVertices[i].X-PointonNewLine.X)/cutvector.X;
				}
				else
				{
					if (abs(Edges[i].Y)>tolerance)
					{
						factor = (RawVertices[i].Y-PointonNewLine.Y)/cutvector.Y;
					}
					else
					{
						factor = (RawVertices[i].Z-PointonNewLine.Z)/cutvector.Z;
					}
				}
				if ((PointonNewLine + factor*cutvector).Equals(RawVertices[i],tolerance)&&factor>=0)
				{
					FString a = PointonNewLine.ToString();
					FString b = cutvector.ToString();
					FString c = RawVertices[i].ToString();
					//UE_LOG(LogTemp,Warning,TEXT("PointonNewLine Echt Parallel with factor: %f, Pointon: %s, Cutvec: %s, RawVer: %s"), factor,*a,*b,*c);
					cuts++;
				}
			}
		}

		if(cuts%2==0) 
		{
			//UE_LOG(LogTemp, Display, TEXT("New Line outside %d"),cuts);
			return false;
		}
	return true;
}

bool PolygonConstructor::IsOnePlane(TArray<FVector> RawVertices)
{
	FVector PlaneNormal = FVector::CrossProduct(RawVertices[1].operator-(RawVertices[0]),RawVertices[2].operator-(RawVertices[0]));
	double Product = FVector::DotProduct(PlaneNormal,RawVertices[0]);
	for (int i = 3; i < RawVertices.Num(); ++i)
	{
		double check = FVector::DotProduct(PlaneNormal,RawVertices[i]);
		if (!(abs(check-Product)<tolerance))return false;
	}
	return true;
}

bool PolygonConstructor::Verticesto2D(TArray<FVector> RawVertices)
{
	FVector PlaneNormal = FVector::CrossProduct(RawVertices[1].operator-(RawVertices[0]),RawVertices[2].operator-(RawVertices[0]));
	double Product = FVector::DotProduct(PlaneNormal,RawVertices[0]);
	
	FPlane XYPlane = *new FPlane(0,0,1,0);
	FPlane Plane1 = *new FPlane(PlaneNormal.X,PlaneNormal.Y,PlaneNormal.Z,Product);
	double angle = acos((FVector::DotProduct(Plane1.GetNormal(),XYPlane.GetNormal())/(Plane1.GetNormal().Length()*XYPlane.GetNormal().Length())));
	FVector Intersect = Plane1.GetNormal().operator^(XYPlane.GetNormal());
	Intersect.Normalize();
	FQuat quat = *new FQuat(Intersect,angle);
	FVector Pointsmovedby = *new FVector(0.0,Plane1.W/-Plane1.Y,0.0);

	
	
	for (int index = 0; index < RawVertices.Num(); index++)
	{
		if (!quat.Equals(*new FQuat(0,0,0,0)))
		{
			FVector VertexToAdd = quat.ToMatrix().TransformPosition(RawVertices[index].operator-(Pointsmovedby));
			FVector2D VectortoAdd = *new FVector2D(VertexToAdd.X,VertexToAdd.Y); 
			UVs.Add(VectortoAdd/(scaleValue*50));
		}
		else
		{
			FVector2D VectortoAdd = *new FVector2D(RawVertices[index].X,RawVertices[index].Y); 
			UVs.Add(VectortoAdd/(scaleValue*50));
		}
	}

	

	return true;
}

UStaticMesh* PolygonConstructor::ConvertToStaticMesh(UProceduralMeshComponent* ProcMeshComp, FString Filename, int BuildingNumber)
{
	if (ProcMeshComp != nullptr)
	{
		FString NewNameSuggestion = Filename;
		FString PackageName = FString(TEXT("/Game/Meshes/CityGmlBuildings/")) + NewNameSuggestion + FString::FromInt(BuildingNumber);
		FString Name;
		
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

		FName MeshName(*FPackageName::GetLongPackageAssetName(PackageName));

		if (MeshName == NAME_None)
		{
			AssetToolsModule.Get().CreateUniqueAssetName(PackageName, TEXT(""), PackageName, Name);
			MeshName = *Name;
		}


		FMeshDescription MeshDescription = BuildMeshDescription(ProcMeshComp);

		if (MeshDescription.Polygons().Num() > 0)
		{
			UPackage* Package = CreatePackage(*PackageName);
			check(Package);

			UStaticMesh* StaticMesh = NewObject<UStaticMesh>(Package, MeshName, RF_Public | RF_Standalone);
			StaticMesh->InitResources();

			StaticMesh->SetLightingGuid();

			FStaticMeshSourceModel& SrcModel = StaticMesh->AddSourceModel();
			SrcModel.BuildSettings.bRecomputeNormals = true;
			SrcModel.BuildSettings.bRecomputeTangents = false;
			SrcModel.BuildSettings.bRemoveDegenerates = false;
			SrcModel.BuildSettings.bUseHighPrecisionTangentBasis = false;
			SrcModel.BuildSettings.bUseFullPrecisionUVs = false;
			SrcModel.BuildSettings.bGenerateLightmapUVs = true;
			SrcModel.BuildSettings.SrcLightmapIndex = 0;
			SrcModel.BuildSettings.DstLightmapIndex = 1;
			StaticMesh->CreateMeshDescription(0, MoveTemp(MeshDescription));
			StaticMesh->CommitMeshDescription(0);

			if (!ProcMeshComp->bUseComplexAsSimpleCollision)
			{
				StaticMesh->CreateBodySetup();
				UBodySetup* NewBodySetup = StaticMesh->GetBodySetup();
				NewBodySetup->BodySetupGuid = FGuid::NewGuid();
				NewBodySetup->AggGeom.ConvexElems = ProcMeshComp->ProcMeshBodySetup->AggGeom.ConvexElems;
				NewBodySetup->bGenerateMirroredCollision = false;
				NewBodySetup->bDoubleSidedGeometry = true;
				NewBodySetup->CollisionTraceFlag = CTF_UseDefault;
				NewBodySetup->CreatePhysicsMeshes();
			}

			TSet<UMaterialInterface*> UniqueMaterials;
			const int32 NumSections = ProcMeshComp->GetNumSections();
			for (int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
			{
				FProcMeshSection* ProcSection =
					ProcMeshComp->GetProcMeshSection(SectionIdx);
				UMaterialInterface* Material = ProcMeshComp->GetMaterial(SectionIdx);
				UniqueMaterials.Add(Material);
			}
			for (auto* Material : UniqueMaterials)
			{
				StaticMesh->GetStaticMaterials().Add(FStaticMaterial(Material));
			}

			StaticMesh->ImportVersion = EImportStaticMeshVersion::LastVersion;

			StaticMesh->Build(false);
			StaticMesh->PostEditChange();
			FAssetRegistryModule::AssetCreated(StaticMesh);
			return StaticMesh;
		}
	}
	return nullptr;
}



