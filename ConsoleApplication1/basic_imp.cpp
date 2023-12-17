#include <memory>

#include "shader_m2.h"
#include "primitive_m2.h"
#include "input_m2.h"
#include "primitive_move_m2.h"
#include "light_m2.h"



#define kfAssert(...)


void BVHTreeNode::TryExpand(const BVHTreeNode& InChild)
{
	if (!IsValid())
	{
		Min = InChild.Min;
		Max = InChild.Max;
	}

	Min = glm::min(Min, InChild.Min);
	Max = glm::max(Max, InChild.Max);
}

void BVHTreeNode::DestructNode(const BVHTreeKey& Node)
{
	if (Node.Tree.IsValid())
	{
		Node.Tree->Remove(&Node.Tree->Nodes[Node.Index]);
	}
}

bool BVHTreeNode::Contains(const BVHTreeNode& InOther) const
{
	return InOther.Min.x >= Min.x && InOther.Min.y >= Min.y && InOther.Min.z >= Min.z &&
		InOther.Max.x <= Min.x && InOther.Max.y <= Min.y && InOther.Max.z <= Min.z;
}

bool BVHTreeNode::Overlap(const BVHTreeNode& InOther) const
{
	if (Max.x < InOther.Min.x || Min.x > InOther.Max.x)
	{
		return false;
	}
	if (Max.y < InOther.Min.y || Min.y > InOther.Max.y)
	{
		return false;
	}
	if (Max.z < InOther.Min.z || Min.z > InOther.Max.z)
	{
		return false;
	}

	return true;
}

glm::vec3 BVHTreeNode::Center() const
{
	return (Min + Max) * glm::vec3(0.5f,0.5f,0.5f);
}


size_t BVHTree::NewNode()
{
	if (Nodes.size() == 0)
	{
		Nodes.emplace_back();
	}
	{
		size_t NewIndex;
		if (UnusedNodeIndex.empty())
		{
			NewIndex = Nodes.size();
			Nodes.emplace_back();
		}
		else
		{
			NewIndex = UnusedNodeIndex[UnusedNodeIndex.size() - 1];
			UnusedNodeIndex.pop_back();
		}
		auto* Ret = &Nodes[NewIndex];
		Ret->Tree = this;
		Ret->MyIndex = NewIndex;
		return NewIndex;
	}
}

void BVHTree::DeleteNode(BVHTreeNode* InNode)
{
	{
		kfAssert(!InNode->LeftChild && !InNode->RightChild && InNode->Tree);
		if (InNode->GetParent())
		{
			if (InNode->GetParent()->GetLeft() == InNode)
			{
				InNode->GetParent()->LeftChild = 0;
			}
			else
			{
				kfAssert(InNode->GetParent()->GetRight() == InNode);
				InNode->GetParent()->RightChild = 0;
			}
		}
		if (auto LockedBlock = InNode->Block.lock())
		{
			BVHTreeKey Key;
			Key.Index = InNode->MyIndex;
			Key.Tree = BVHTreeWeakPtrCanAsKey::BuildKeyForSearch(InNode->Tree);
			LockedBlock->GetAABBTreeKeys().erase(Key);
			//InNode->Block->m_AABBTreeNodes.erase(InNode);
		}
		InNode->Tree->UnusedNodeIndex.push_back(InNode->MyIndex);
		InNode->Tree = nullptr;
		InNode->Parent = 0;
		InNode->Block.reset();
	}
}

void BVHTree::RefreshNode(size_t TargetIndex)
{
	auto* Node = &Nodes[TargetIndex];
	bool bNeedRefresh = !Node->Parent && Node->MyIndex > 1;
	if (auto LockedBlock = Node->Block.lock())
	{
		//auto&& Box = LockedBlock->GetBox();
		BVHBound Bound = LockedBlock->GetBox();
		if (Node->Min != Bound.begin || Node->Max != Bound.end)
		{
			Node->Min = Bound.begin;
			Node->Max = Bound.end;
			bNeedRefresh = true;
		}
	}
	if (bNeedRefresh)
	{
		Remove(Node, false);
		FindPosition(GetRootIndex(), TargetIndex);
	}
}

void BVHTree::FindPosition(size_t InParentIndex, size_t InNewNodeIndex)
{
	BVHTreeNode* InParent = &Nodes[InParentIndex];
	BVHTreeNode* InNewNode = &Nodes[InNewNodeIndex];
	kfAssert(
		(InParent->Block && !InParent->LeftChild && !InParent->RightChild) ||
		(!InParent->Block)
	);

	if (auto LockedBlock = InParent->Block.lock())// 如果是叶子节点
	{
		auto* NewParent = &Nodes[NewNode()];
		//may reallocate
		InParent = &Nodes[InParentIndex];
		InNewNode = &Nodes[InNewNodeIndex];

		AssignNode(NewParent, InParent);

		NewParent->Block.reset();

		if (InParent->GetParent()->GetLeft() == InParent)
		{
			InParent->GetParent()->LeftChild = NewParent->MyIndex;
		}
		else
		{
			kfAssert(InParent->GetParent()->GetRight() == InParent);
			InParent->GetParent()->RightChild = NewParent->MyIndex;
		}

		NewParent->LeftChild = InParent->MyIndex;
		NewParent->RightChild = InNewNode->MyIndex;

		InParent->Parent = NewParent->MyIndex;
		InNewNode->Parent = NewParent->MyIndex;

		NewParent->TryExpand(*InNewNode);
		return;//InParent = NewParent;
	}

	kfAssert(InParent->LeftChild || InParent->RightChild || InParent == GetRoot());

	BVHTreeNode LeftNodeTemp;
	if (InParent->LeftChild)
	{
		LeftNodeTemp.TryExpand(*InParent->GetLeft());
	}
	BVHTreeNode RightNodeTemp;
	if (InParent->RightChild)
	{
		RightNodeTemp.TryExpand(*InParent->GetRight());
	}



	float LeftVolume = LeftNodeTemp.Volume();
	float RightVolume = RightNodeTemp.Volume();

	LeftNodeTemp.TryExpand(*InNewNode);
	RightNodeTemp.TryExpand(*InNewNode);

	float LeftAndNewVolume = LeftNodeTemp.Volume();
	float RightAndNewVolume = RightNodeTemp.Volume();


	if (LeftAndNewVolume + RightVolume < RightAndNewVolume + LeftVolume)
	{
		if (InParent->LeftChild)
		{
			FindPosition(InParent->GetLeft()->MyIndex, InNewNode->MyIndex);
			//may reallocate
			InParent = &Nodes[InParentIndex];
			InNewNode = &Nodes[InNewNodeIndex];
		}
		else
		{
			InParent->LeftChild = InNewNode->MyIndex;
			InNewNode->Parent = InParent->MyIndex;
		}
	}
	else if (LeftAndNewVolume + RightVolume > RightAndNewVolume + LeftVolume)
	{
		if (InParent->RightChild)
		{
			FindPosition(InParent->GetRight()->MyIndex, InNewNode->MyIndex);
			//may reallocate
			InParent = &Nodes[InParentIndex];
			InNewNode = &Nodes[InNewNodeIndex];
		}
		else
		{
			InParent->RightChild = InNewNode->MyIndex;
			InNewNode->Parent = InParent->MyIndex;
		}
	}
	else //无论加入哪边增加的总体积都相等
	{
		if (InParent->LeftChild && InParent->RightChild)
		{

			glm::vec3 NewCenter = InNewNode->Center();
			glm::vec3 LeftCenter = InParent->GetLeft()->Center();
			glm::vec3 RightCenter = InParent->GetRight()->Center();

			if (glm::distance(NewCenter, LeftCenter) < glm::distance(NewCenter, RightCenter))//若离着左边近
			{
				if (InParent->LeftChild)
				{
					FindPosition(InParent->GetLeft()->MyIndex, InNewNode->MyIndex);
					//may reallocate
					InParent = &Nodes[InParentIndex];
					InNewNode = &Nodes[InNewNodeIndex];
				}
				else
				{
					InParent->LeftChild = InNewNode->MyIndex;
					InNewNode->Parent = InParent->MyIndex;
				}
			}
			else
			{
				if (InParent->RightChild)
				{
					FindPosition(InParent->GetRight()->MyIndex, InNewNode->MyIndex);
					//may reallocate
					InParent = &Nodes[InParentIndex];
					InNewNode = &Nodes[InNewNodeIndex];
				}
				else
				{
					InParent->RightChild = InNewNode->MyIndex;
					InNewNode->Parent = InParent->MyIndex;
				}
			}
		}
		else if (!InParent->LeftChild)
		{
			InParent->LeftChild = InNewNode->MyIndex;
			InNewNode->Parent = InParent->MyIndex;
		}
		else
		{
			InParent->RightChild = InNewNode->MyIndex;
			InNewNode->Parent = InParent->MyIndex;
		}

	}
	InParent->TryExpand(*InNewNode);
}

void BVHTree::DeleteTree(BVHTreeNode* InNode)
{
	if (InNode->LeftChild)
	{
		DeleteTree(InNode->GetLeft());
		InNode->LeftChild = 0;
	}
	if (InNode->RightChild)
	{
		DeleteTree(InNode->GetRight());
		InNode->RightChild = 0;
	}

	DeleteNode(InNode);
}

void BVHTree::Remove(BVHTreeNode* Node, bool bDeleteNode)
{
	kfAssert(!Node->LeftChild && !Node->RightChild);

	if (!Node->Parent)
	{
		return;
	}

	{
		BVHTreeNode* Brother;

		if (Node->GetParent()->GetLeft() == Node)
		{
			Node->GetParent()->LeftChild = 0;
			Brother = Node->GetParent()->GetRight();
		}
		else
		{
			kfAssert(Node->GetParent()->GetRight() == Node);
			Node->GetParent()->RightChild = 0;
			Brother = Node->GetParent()->GetLeft();
		}



		auto* Parent = Node->GetParent();
		auto* GrandParent = Node->GetParent()->GetParent();
		Node->Parent = 0;

		if (!Brother)
		{
			if (Parent != GetRoot())
			{
				Remove(Parent);
			}
			else
			{
				Parent->Reset();
			}
		}
		else
		{
			if (Parent != GetRoot())
			{
				Brother->Parent = GrandParent->MyIndex;
				if (GrandParent->GetLeft() == Parent)
				{
					GrandParent->LeftChild = Brother->MyIndex;
				}
				else
				{
					kfAssert(GrandParent->GetRight() == Parent);
					GrandParent->RightChild = Brother->MyIndex;
				}
				Parent->Parent = 0;
				Parent->RightChild = 0;
				Parent->LeftChild = 0;

				DeleteNode(Parent);

				auto* RecusivelyParent = GrandParent;
				while (RecusivelyParent)
				{
					RecusivelyParent->Reset();
					if (RecusivelyParent->LeftChild)
					{
						RecusivelyParent->TryExpand(*RecusivelyParent->GetLeft());
					}
					if (RecusivelyParent->RightChild)
					{
						RecusivelyParent->TryExpand(*RecusivelyParent->GetRight());
					}
					RecusivelyParent = RecusivelyParent->GetParent();
				}
			}
			else
			{
				Parent->Reset();
				Parent->TryExpand(*Brother);
			}
		}
		if (bDeleteNode)
		{
			DeleteNode(Node);
		}
		else
		{
			//Node->Refresh();
		}

	}
}

size_t BVHTree::Build(std::vector<std::weak_ptr<IBVHTreeItem>>::iterator Begin,
	std::vector<std::weak_ptr<IBVHTreeItem>>::iterator End, bool bDontSliceByZ)
{
	auto LockedBegin = Begin->lock();
	auto LockedEnd = End->lock();
	if (Begin == End)
	{
		return 0;
	}
	if (End - Begin == 1)//叶子节点
	{
		//if (Begin->Get()->m_AABBTreeNodes.find())
		//{
		//	kfAssert(Begin->Get()->m_AABBTreeNode->Tree != this);
		//	BVHTreeNode::DestructNode(Begin->Get()->m_AABBTreeNode.Get());
		//}

		BVHTreeNode* Ret = &Nodes[NewNode()];

		BVHBound Bound = LockedBegin->GetBox();
		Ret->Min = Bound.begin;
		Ret->Max = Bound.end;
		Ret->Block = LockedBegin;
		LockedBegin->GetAABBTreeKeys().insert({ Ret->MyIndex, Ret->Tree });
		return Ret->MyIndex;
	}

	BVHTreeNode AllBounding;

	for (auto It = Begin; It != End; ++It)
	{
		auto LockedIt = It->lock();
		BVHBound Bound = LockedIt->GetBox();
		BVHTreeNode Temp;
		Temp.Min = Bound.begin;
		Temp.Max = Bound.end;

		AllBounding.TryExpand(Temp);
	}

	glm::vec3 Delta = AllBounding.Max - AllBounding.Min;

	if (bDontSliceByZ)
	{
		if (Delta.x >= Delta.y)
		{
			std::sort(Begin, End, [&](std::weak_ptr<IBVHTreeItem>& A, std::weak_ptr<IBVHTreeItem>& B)
				{
					return A.lock()->GetBox().Center().x < B.lock()->GetBox().Center().x;
				});
		}
		else
		{
			std::sort(Begin, End, [&](std::weak_ptr<IBVHTreeItem>& A, std::weak_ptr<IBVHTreeItem>& B)
				{
					return A.lock()->GetBox().Center().y < B.lock()->GetBox().Center().y;
				});
		}
	}
	else
	{
		if (Delta.x >= Delta.y && Delta.x >= Delta.z)
		{
			std::sort(Begin, End, [&](std::weak_ptr<IBVHTreeItem>& A, std::weak_ptr<IBVHTreeItem>& B)
				{
					return A.lock()->GetBox().Center().x < B.lock()->GetBox().Center().x;
				});
		}
		else if (Delta.y >= Delta.x && Delta.y >= Delta.z)
		{
			std::sort(Begin, End, [&](std::weak_ptr<IBVHTreeItem>& A, std::weak_ptr<IBVHTreeItem>& B)
				{
					return A.lock()->GetBox().Center().y < B.lock()->GetBox().Center().y;
				});
		}
		else
		{
			std::sort(Begin, End, [&](std::weak_ptr<IBVHTreeItem>& A, std::weak_ptr<IBVHTreeItem>& B)
				{
					return A.lock()->GetBox().Center().z < B.lock()->GetBox().Center().z;
				});
		}
	}


	auto Mid = Begin + (End - Begin) / 2;

	size_t RetIndex = NewNode();

	auto LeftiChildIndex = Build(Begin, Mid, bDontSliceByZ);
	auto RightChildIndex = Build(Mid, End, bDontSliceByZ);

	Nodes[RetIndex].LeftChild = LeftiChildIndex;
	Nodes[RetIndex].RightChild = RightChildIndex;

	BVHTreeNode* Ret = &Nodes[RetIndex];

	if (Ret->LeftChild)
	{
		Ret->GetLeft()->Parent = Ret->MyIndex;
		Ret->TryExpand(*Ret->GetLeft());
	}
	if (Ret->RightChild)
	{
		Ret->GetRight()->Parent = Ret->MyIndex;
		Ret->TryExpand(*Ret->GetRight());
	}
	return RetIndex;
}

void BVHTree::ExchangeNodeTree(BVHTreeNode* Node)
{
	if (Node->LeftChild)
	{
		ExchangeNodeTree(Node->GetLeft());
	}
	if (Node->RightChild)
	{
		ExchangeNodeTree(Node->GetRight());
	}
	Node->Tree = this;
	if (auto LockedBlock = Node->Block.lock())
	{
		LockedBlock->GetAABBTreeKeys().insert({ Node->MyIndex, Node->Tree });
	}
}

static bool AABBAABBTest(glm::vec3 InMin, glm::vec3 InMax, glm::vec3 InMin2, glm::vec3 InMax2)
{

	if (InMax2.x < InMin.x || InMin2.x > InMax.x)
	{
		return false;
	}
	if (InMax2.y < InMin.y || InMin2.y > InMax.y)
	{
		return false;
	}
	if (InMax2.z < InMin.z || InMin2.z > InMax.z)
	{
		return false;
	}
	return true;
}

void BVHTree::GetBlocksMayOverlap(glm::vec3 InMin, glm::vec3 InMax, const BVHTreeNode* InParent,
	std::vector<std::weak_ptr<IBVHTreeItem>>& Out) const
{
	if (InParent)
	{
		if (AABBAABBTest(InMin, InMax, InParent->Min, InParent->Max))
		{
			if (InParent->LeftChild || InParent->RightChild)
			{
				GetBlocksMayOverlap(InMin, InMax, InParent->GetLeft(), Out);
				GetBlocksMayOverlap(InMin, InMax, InParent->GetRight(), Out);
			}
			else if (auto LockedBlock =  InParent->Block.lock())
			{
				Out.push_back(InParent->Block);
			}
		}
	}
}

void BVHTree::ConstructByBlock(size_t Index, std::weak_ptr<IBVHTreeItem> InBlock)
{
	auto* Node = &Nodes[Index];
	Node->Block = InBlock;
	InBlock.lock()->GetAABBTreeKeys().insert({ Index, this });

	RefreshNode(Index);
}


BVHTree::BVHTree(const BVHTree& Other) : Nodes(Other.Nodes)
{
	if (GetRoot())
	{
		ExchangeNodeTree(GetRoot());
	}
}

std::shared_ptr<BVHTree> BVHTree::CreateTree()
{
	KFAABBTreeSharedPtrParam Param;
	return std::make_shared<BVHTree>(Param);
}

std::vector<std::weak_ptr<IBVHTreeItem>> BVHTree::GetBlocksMayOverlap(glm::vec3 InMin, glm::vec3 InMax) const
{
	std::vector<std::weak_ptr<IBVHTreeItem>> Ret;
	GetBlocksMayOverlap(InMin, InMax, GetRoot(), Ret);
	return Ret;
}

BVHTree& BVHTree::operator=(const BVHTree& Other)
{
	Nodes = Other.Nodes;
	;
	if (GetRoot())
	{
		ExchangeNodeTree(GetRoot());
	}
	return *this;
}

void BVHTree::Insert(std::weak_ptr<IBVHTreeItem> NewBlock)
{
	if (!GetRoot())
	{
		size_t RU = NewNode();
		kfAssert(RU == GetRootIndex());
	}

	//if(NewBlock->m_AABBTreeNode)
	//{
	//	return;
	//}

	size_t B = NewNode();
	ConstructByBlock(B, NewBlock);

	//FindPosition(Root, B);
}

void BVHTree::Remove(std::weak_ptr<IBVHTreeItem> InBlock)
{
	auto LockedBlock = InBlock.lock();
	auto ONodes = LockedBlock->GetAABBTreeKeys();
	for (auto&& It = ONodes.begin(); It != ONodes.end(); ++It)
	{
		auto&& ONode = It->Tree->Nodes[It->Index];

		if (ONode.Tree == this)
		{
			Remove(&ONode);
		}
	}

	//if (InBlock->m_AABBTreeNode && InBlock->m_AABBTreeNode->Tree == this)
	//{
	//	Remove(InBlock->m_AABBTreeNode.Get());
	//	InBlock->m_AABBTreeNode = nullptr;
	//}
}

void BVHTree::Build(std::vector<std::weak_ptr<IBVHTreeItem>>& Blocks, bool bDontSliceByZ)
{
	if (GetRoot())
	{
		Destroy();
	}

	if (Blocks.size() == 0)
	{
		size_t RU = NewNode();
		kfAssert(RU == 1);
		return;
	}

	if (Blocks.size() == 1)
	{
		//Root = NewNode();
		//if(Blocks[0]->m_AABBTreeNode)
		//{
		//	kfAssert(Blocks[0]->m_AABBTreeNode->Tree != this);
		//	BVHTreeNode::DestructNode(Blocks[0]->m_AABBTreeNode.Get());
		//}

		//KFBound Bound = Blocks[0]->GetBox().GetAABBBound();
		//Root->Min = Bound.begin;
		//Root->Max = Bound.end;
		//Root->Block = Blocks[0];

		Insert(Blocks[0]);
	}
	else
	{
		size_t RU = Build(Blocks.begin(), Blocks.end(), bDontSliceByZ);
		kfAssert(RU == 1);
	}
}

void BVHTree::DrawDebug(glm::mat4 v, glm::mat4 p, glm::vec3 cameraPos)
{
	static bool Init = false;
	static FPrimitiveRef DebugPrim = std::make_shared<FPrimitive>();
	static FShaderRef DebugShader = std::make_shared<FShader>("shaders/debug_shader.vs","shaders/debug_shader.fs");
	if(!Init)
	{
		float vertices[] = {
	0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
	 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
	 -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
	0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

	-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
	 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
	 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
	 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
	-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
	-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

	-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
	-0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
	-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
	-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

	0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
	 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
	 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
	 0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
	0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
	 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,


	-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
	0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
	 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
	 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
	-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

	0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
	-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
	 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
	 -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
	0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
	-0.5f,  0.5f, -0.5f,  0.0f, 1.0f
		};


		DebugShader->setVec4("InputColor", glm::vec4(1, 0, 0, 1));
		
		std::vector<char> vertexData;
		vertexData.resize(sizeof(vertices));
		memcpy(vertexData.data(), vertices, vertexData.size());
		FPrimitiveVertexDesc vertexDesc;
		vertexDesc.structSize = 5 * sizeof(int);
		vertexDesc.props.emplace_back(0, reinterpret_cast<void*>(0), GL_FLOAT, 3);
		vertexDesc.props.emplace_back(2, reinterpret_cast<void*>(3 * sizeof(float)), GL_FLOAT, 2);

		//创建一个立方体模型
		FPrimitiveRef cube = std::make_shared<FPrimitive>();
		DebugPrim->SetData(vertexData, std::vector<unsigned int>(), vertexDesc);

		Init = true;
	}

	

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	for(auto&& Node : Nodes)
	{
		glm::mat4 sc = glm::scale(glm::mat4(1), glm::vec3(Node.Max - Node.Min));
		glm::mat4 model = glm::translate(sc, Node.Center());
		
		FRenderBatch Batch;
		Batch.Shader = DebugShader;
		Batch.Primitive = DebugPrim;
		Batch.model = model;
		Batch.PrimitiveMode = GL_LINE_LOOP;
		Batch.Draw_InputVP(v, p, cameraPos);
	}

}

void BVHTree::Destroy()
{
	if (GetRoot())
	{
		DeleteTree(GetRoot());


		Nodes.clear();
		Nodes.emplace_back();
		UnusedNodeIndex.clear();
	}
}

void FSceneComponent::UpdateBoundCache()
{
	if(ShouldAdd())
	{
		auto LastBound = BoundCache;
		OnUpdateBoundCache();
		if (BoundCache.begin != LastBound.begin || BoundCache.end != LastBound.end)
		{
			scene.lock()->UpdateBVH(GetBVHItem());
		}
	}
}

void FSceneComponent::FinalTick(float deltaSecond)
{
	FComponent::FinalTick(deltaSecond);



}

void FScene::DrawDebug(glm::mat4 v, glm::mat4 p, glm::vec3 cp)
{
	Tree->DrawDebug(v, p, cp);
}


std::shared_ptr<FShader> FDirectionalLightComponent::DirectionalLightDeferredShader;
std::shared_ptr<FPrimitive> FDirectionalLightComponent::DirectionalLightDeferredGeo;

std::shared_ptr<FShader> FEnvLightComponent::EnvLightDeferredShader;
std::shared_ptr<FPrimitive> FEnvLightComponent::EnvLightDeferredGeo;

std::shared_ptr<FShader> FPointLightComponent::PointLightDeferredShader;
std::shared_ptr<FPrimitive> FPointLightComponent::PointLightDeferredGeo;

std::shared_ptr<FShader> FCameraComponent::FinalShader;
std::shared_ptr<FPrimitive> FCameraComponent::FinalPrimitive;

FInputReceiver& FInputReceiver::GetInputReceiver()
{
    static FInputReceiver innerReceiver;
    return innerReceiver;
}

long long FObject::GenComponentID()
{
    static long long int curId = 0;
    return curId++;
}

FCameraComponent::FDeferredDrawer& FCameraComponent::GetDeferredCmds()
{
    static FDeferredDrawer innerDrawer;
    return innerDrawer;
}

void FCameraComponent::AdjustGBuffer()
{
    //if(bDeferredPipeline)
    {
        FFrameBufferRef useFrameBuffer = frameBufferRef ? frameBufferRef : FFrameBuffer::GetDefaultFrameBuffer();
        bool bViewportSet = false;
        glm::vec2 viewportSize;
        if (!useFrameBuffer->IsEmpty())
        {
            if (useFrameBuffer->Color[0]->IsValid())
            {
                viewportSize = useFrameBuffer->Color[0]->GetSize();
                bViewportSet = true;
            }
            else if (useFrameBuffer->Depth->IsValid())
            {
                viewportSize = useFrameBuffer->Depth->GetSize();
                bViewportSet = true;
            }
        }
        if (!bViewportSet)
        {
            int x, y;
            glfwGetFramebufferSize(glfwGetCurrentContext(), &x, &y);
            viewportSize.x = x;
            viewportSize.y = y;
        }
        if (bDeferredPipeline)
        {
            if (!gBufferRef || gBufferRef->Color[0]->GetSize() != viewportSize)
            {
                gBufferRef = std::make_shared<FFrameBuffer>(static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y), 4, EFrameBufferColorFormat::FCF_RGBA16F);
                gFlipBufferRefs[0] = std::make_shared<FFrameBuffer>(static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y), 1, EFrameBufferColorFormat::FCF_RGBA);
                //gFlipBufferRefs[1] = std::make_shared<FFrameBuffer>(static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y), 1, EFrameBufferColorFormat::FCF_RGBA);
            }
        }
        else
        {
            if (!gFlipBufferRefs[0] || gFlipBufferRefs[0]->Color[0]->GetSize() != viewportSize)
            {
                gFlipBufferRefs[0] = std::make_shared<FFrameBuffer>(static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y), 1, EFrameBufferColorFormat::FCF_RGBA);
                //gFlipBufferRefs[1] = std::make_shared<FFrameBuffer>(static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y), 1, EFrameBufferColorFormat::FCF_RGBA);
            }
        }
        
    }
    //else
    //{
    //    gBufferRef = nullptr;
    //}
}

void FCameraComponent::Init(glm::vec3 position)
{
	SetWorldLocation(position);
}

void FCameraComponent::DrawDeferred() const
{
    auto&& drawer = GetDeferredCmds();
    if(drawer.registeredCamera.find(this) == drawer.registeredCamera.end())
    {
        drawer.registeredCamera.emplace(this);

        std::weak_ptr<const FCameraComponent> weakThis = std::static_pointer_cast<const FCameraComponent>(this->GetObject());

        if (!frameBufferRef || frameBufferRef->IsEmpty())
        {
            drawer.deferredCommands.emplace([weakThis]()->void
                {
                    auto safeCamera = weakThis.lock();
                    if (safeCamera)
                    {
                        safeCamera->Draw();
						auto LockedScene = safeCamera->scene.lock();
						if(LockedScene)
						{
							LockedScene->DrawDebug(safeCamera->GetViewMatrix(), safeCamera->GetProjectionMatrix(), safeCamera->GetLocalLocation());
						}
                    }
                });
        }
        else
        {
            drawer.preDeferredCommands.emplace([weakThis]()->void
                {
                    auto safeCamera = weakThis.lock();
                    if (safeCamera)
                    {
                        safeCamera->Draw();
						auto LockedScene = safeCamera->scene.lock();
						if (LockedScene)
						{
							LockedScene->DrawDebug(safeCamera->GetViewMatrix(), safeCamera->GetProjectionMatrix(), safeCamera->GetLocalLocation());
						}
                    }
                });
        }
    	
    }
}

void FCameraComponent::Draw() const
{
    std::vector<FRenderBatch> renderBatches;
    std::vector<FLightRenderBatch> lights;

    auto safe_scene = scene.lock();
    auto&& allComponents = safe_scene->GetAllComponents();
    for (auto&& component : allComponents)
    {
        auto primitiveComponent = std::dynamic_pointer_cast<FPrimitiveComponent>(component);
        if (primitiveComponent)
        {
            if(renderOnlyPrimitives.size() == 0)
            {
                if (ignorePrimitives.find(primitiveComponent) == ignorePrimitives.end())
                {
                    primitiveComponent->GenerateRenderBatch(renderBatches);
                }
            }
            else
            {
                if (renderOnlyPrimitives.find(primitiveComponent) != renderOnlyPrimitives.end())
                {
                    primitiveComponent->GenerateRenderBatch(renderBatches);
                }
            }
        }
        else
        {
            auto lightComponent = std::dynamic_pointer_cast<FLightComponent>(component);
            if(lightComponent)
            {
                lightComponent->GetLightRenderBatch(lights);
            }
        }
    }
    FFrameBufferRef useFrameBuffer = frameBufferRef ? frameBufferRef : FFrameBuffer::GetDefaultFrameBuffer();

    bool bViewportSet = false;
    glm::vec2 viewportSize;
    if(!useFrameBuffer->IsEmpty())
    {
        if(useFrameBuffer->Color[0]->IsValid())
        {
            viewportSize = useFrameBuffer->Color[0]->GetSize();
            bViewportSet = true;
        }
        else if(useFrameBuffer->Depth->IsValid())
        {
            viewportSize = useFrameBuffer->Depth->GetSize();
            bViewportSet = true;
        }
    }
    if(!bViewportSet)
    {
        int x, y;
        glfwGetFramebufferSize(glfwGetCurrentContext(), &x, &y);
        viewportSize.x = x;
        viewportSize.y = y;
    }

    glm::vec4 clearColor = useFrameBuffer->clearColor;
    const_cast<FCameraComponent*>(this)->AdjustGBuffer();
    if(bDeferredPipeline)
    {

        //draw shadow maps
#if 1
        for (auto&& light : lights)
        {
            switch (light.lightType)
            {
            case ELightType::LT_Directional:
                if (light.shadowMap)
                {
                    light.shadowMap->Use();
                    auto shadowMapSize = light.shadowMap->Depth->GetSize();
                    glViewport(0, 0, shadowMapSize.x, shadowMapSize.y);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
                    glEnable(GL_DEPTH_TEST);
                    float zoomInRadius = glm::radians(Zoom);
                    float tanHalfZoom = glm::tan(zoomInRadius / 2.0f);
                    auto bufferSize = gBufferRef->Color[0]->GetSize();
                    float aspectRatio = bufferSize.x / bufferSize.y;
                    for (int CSMIndex = 0; CSMIndex < light.numOfCSM; ++CSMIndex)
                    { 
                        glViewport(0, 0, shadowMapSize.x, shadowMapSize.x * (CSMIndex + 1));
                        float CSMInnerFarPlane = ((CSMIndex + 1) / (float)light.numOfCSM) * light.lightmapDistance;
                        float CSMInnerNearPlane = ((CSMIndex) / (float)light.numOfCSM) * light.lightmapDistance;
                         
                        float lhalfHight = CSMInnerFarPlane * tanHalfZoom; 
                        float lhalfWidth = lhalfHight * aspectRatio;
                        float lHalfLength = (CSMInnerFarPlane - CSMInnerNearPlane) / 2;
                        float CSMRadius = glm::sqrt(lhalfHight * lhalfHight + lhalfWidth * lhalfWidth + lHalfLength * lHalfLength);
                         

                        glm::vec3 shadowCasterPos = GetWorldLocation() + GetFowardInWorldSpace() * (CSMInnerNearPlane + CSMInnerFarPlane) * 0.5f - light.direction * 5.0f;
                        glm::vec3 shadowCasterUp = glm::vec3(0, 1, 0);
                        if (glm::abs(glm::abs(dot(light.direction, shadowCasterUp)) - 1.0f) < 0.05f)
                        {
                            shadowCasterUp = glm::vec3(0, 0, -1);
                        }


                        glm::vec3 casterForward = light.direction;
                    	glm::vec3 casterRight = glm::normalize(glm::cross(casterForward, shadowCasterUp));
                        glm::vec3 casterUp = glm::cross(casterRight, casterForward);

                        glm::vec3 shadowCasterBasePos = GetWorldLocation() + GetFowardInWorldSpace() * (CSMInnerNearPlane + CSMInnerFarPlane) * 0.5f;

                        float rightValue = glm::dot(shadowCasterBasePos, casterRight);
                        float upValue = glm::dot(shadowCasterBasePos, casterUp);
                        float forwardValue = glm::dot(shadowCasterBasePos, casterForward);

                        glm::vec2 cellCount = light.shadowMap->Depth->GetSize();
                        float rightStepSize = 2 * CSMRadius / cellCount.x;
                        float upStepSize = 2 * CSMRadius / cellCount.y;

                        float rightStepValue = glm::floor(rightValue / rightStepSize) * rightStepSize;
                        float upStepValue = glm::floor(upValue / upStepSize) * upStepSize;


                        shadowCasterBasePos = rightStepValue * casterRight + upStepValue * casterUp + forwardValue * light.direction;
                        shadowCasterPos = shadowCasterBasePos - light.direction * 5.0f;

                        glm::mat4 casterView = glm::lookAt(shadowCasterPos, shadowCasterPos + light.direction, shadowCasterUp);

                        glm::mat4 proj = glm::ortho(-CSMRadius, CSMRadius, -CSMRadius, CSMRadius, 1.0f, 10.0f);

                        light.worldToShadowProj[CSMIndex] = proj * casterView;

                        for (auto&& primitve : renderBatches)
                        { 
                            primitve.Draw_InputVP(casterView, proj, shadowCasterPos); 
                        }
                    }

                }
                break;
            case ELightType::LT_Point:

                break;
            case ELightType::LT_Env:
                break;
            }
        }
#endif


    	gBufferRef->Use();
        glViewport(0, 0, static_cast<GLsizei>(viewportSize.x), static_cast<GLsizei>(viewportSize.y));
        glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);


        //draw g-buffer
        for(auto&& renderBatch : renderBatches)
        {
            renderBatch.Draw(std::static_pointer_cast<FCameraComponent>(((FCameraComponent*)this)->GetObject()));
        }
        glUseProgram(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        if (!FDirectionalLightComponent::DirectionalLightDeferredShader)
        {
            FDirectionalLightComponent::DirectionalLightDeferredShader = std::make_shared<FShader>("./shaders/directional_light_deferred.vs", "./shaders/directional_light_deferred.fs");
            FDirectionalLightComponent::DirectionalLightDeferredShader->SetBlendMethod(EBlendMethod::BM_Additive);
        }
        if (!FDirectionalLightComponent::DirectionalLightDeferredGeo)
        {
            FDirectionalLightComponent::DirectionalLightDeferredGeo = std::make_shared<FPrimitive>();

            float geoDatas[] = {
                -1.0f, 1.0f, -0.5f,
                -1.0f, -1.0f,-0.5f,
                1.0f, -1.0f,-0.5f,
                1.0f, 1.0f, -0.5f
            };

            std::vector<char> vertex;
            vertex.resize(sizeof(geoDatas));
            memcpy(vertex.data(), geoDatas, vertex.size());

            std::vector<unsigned int> index = {
                0, 1, 2, 0, 2, 3
            };

            FPrimitiveVertexDesc desc;
            desc.structSize = 3 * sizeof(float);
            FPrimitiveVertexPropDesc prop(0, nullptr, GL_FLOAT, 3);
            desc.props.push_back(prop);

            FDirectionalLightComponent::DirectionalLightDeferredGeo->SetData(vertex, index, desc);
        }

        if (!FEnvLightComponent::EnvLightDeferredGeo)
        {
            FEnvLightComponent::EnvLightDeferredGeo = std::make_shared<FPrimitive>();

            float geoDatas[] = {
                -1.0f, 1.0f, -0.5f,
                -1.0f, -1.0f,-0.5f,
                1.0f, -1.0f,-0.5f,
                1.0f, 1.0f, -0.5f
            };

            std::vector<char> vertex;
            vertex.resize(sizeof(geoDatas));
            memcpy(vertex.data(), geoDatas, vertex.size());

            std::vector<unsigned int> index = {
                0, 1, 2, 0, 2, 3
            };

            FPrimitiveVertexDesc desc;
            desc.structSize = 3 * sizeof(float);
            FPrimitiveVertexPropDesc prop(0, nullptr, GL_FLOAT, 3);
            desc.props.push_back(prop);

            FEnvLightComponent::EnvLightDeferredGeo->SetData(vertex, index, desc);
        }
        if (!FEnvLightComponent::EnvLightDeferredShader)
        {
            FEnvLightComponent::EnvLightDeferredShader = std::make_shared<FShader>("./shaders/env_light_deferred.vs", "./shaders/env_light_deferred.fs");
            FEnvLightComponent::EnvLightDeferredShader->SetBlendMethod(EBlendMethod::BM_Additive);
        }

        if (!FPointLightComponent::PointLightDeferredGeo)
        {
            FPointLightComponent::PointLightDeferredGeo = std::make_shared<FPrimitive>();



			float geoDatas[] = {
				0.5f, -0.5f, -0.5f,  
				-0.5f, -0.5f, -0.5f,  
				0.5f,  0.5f, -0.5f,  
				-0.5f,  0.5f, -0.5f,  
				0.5f,  0.5f, -0.5f,  
				-0.5f, -0.5f, -0.5f,  

				-0.5f, -0.5f,  0.5f,  
				0.5f, -0.5f,  0.5f,  
				0.5f,  0.5f,  0.5f,  
				0.5f,  0.5f,  0.5f,  
				-0.5f,  0.5f,  0.5f,  
				-0.5f, -0.5f,  0.5f,  

				-0.5f,  0.5f,  0.5f,  
				-0.5f,  0.5f, -0.5f,  
				-0.5f, -0.5f, -0.5f,  
				-0.5f, -0.5f, -0.5f,  
				-0.5f, -0.5f,  0.5f,  
				-0.5f,  0.5f,  0.5f,  

				0.5f,  0.5f, -0.5f,  
				0.5f,  0.5f,  0.5f,  
				0.5f, -0.5f, -0.5f,  
				0.5f, -0.5f,  0.5f,  
				0.5f, -0.5f, -0.5f,  
				0.5f,  0.5f,  0.5f,  


				-0.5f, -0.5f, -0.5f,  
				0.5f, -0.5f, -0.5f,  
				0.5f, -0.5f,  0.5f,  
				 0.5f, -0.5f,  0.5f,  
				-0.5f, -0.5f,  0.5f,  
				-0.5f, -0.5f, -0.5f,  

				0.5f,  0.5f, -0.5f,  
				-0.5f,  0.5f, -0.5f,  
				0.5f,  0.5f,  0.5f,  
				-0.5f,  0.5f,  0.5f, 
				0.5f,  0.5f,  0.5f,  
				-0.5f,  0.5f, -0.5f,  
			};




            //float geoDatas[] = {
            //    -1.0f, 1.0f, -0.5f,
            //    -1.0f, -1.0f,-0.5f,
            //    1.0f, -1.0f,-0.5f,
            //    1.0f, 1.0f, -0.5f
            //};

            std::vector<char> vertex;
            vertex.resize(sizeof(geoDatas));
            memcpy(vertex.data(), geoDatas, vertex.size());

            std::vector<unsigned int> index = {
               // 0, 1, 2, 0, 2, 3
            };

            FPrimitiveVertexDesc desc;
            desc.structSize = 3 * sizeof(float);
            FPrimitiveVertexPropDesc prop(0, nullptr, GL_FLOAT, 3);
            desc.props.push_back(prop);

            FPointLightComponent::PointLightDeferredGeo->SetData(vertex, index, desc);
        }
        if (!FPointLightComponent::PointLightDeferredShader)
        {
            FPointLightComponent::PointLightDeferredShader = std::make_shared<FShader>("./shaders/point_light_deferred.vs", "./shaders/point_light_deferred.fs");
            FPointLightComponent::PointLightDeferredShader->SetBlendMethod(EBlendMethod::BM_Additive);
            FPointLightComponent::PointLightDeferredShader->SetCullMethod(ECullMethod::CM_None);
        }


    	gFlipBufferRefs[0]->Use();
        glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    	glDisable(GL_DEPTH_TEST);
        //int curFlipBufferIndex = 0;
        //draw lights;
        gFlipBufferRefs[0]->Use();
        for (auto&& light : lights)
        {
            switch (light.lightType)
            {
            case ELightType::LT_Directional:
	            {

                FDirectionalLightComponent::DirectionalLightDeferredShader->use();
                FDirectionalLightComponent::DirectionalLightDeferredGeo->use();

                FDirectionalLightComponent::DirectionalLightDeferredShader->SetTexture("gWorldPosMetallic", gBufferRef->Color[1]);
                FDirectionalLightComponent::DirectionalLightDeferredShader->SetTexture("gAlbedoSpec", gBufferRef->Color[2]);
                FDirectionalLightComponent::DirectionalLightDeferredShader->SetTexture("gWorldNormalRoughness", gBufferRef->Color[3]);
                FDirectionalLightComponent::DirectionalLightDeferredShader->setVec3("DirectionalLightDir", -light.direction);
                FDirectionalLightComponent::DirectionalLightDeferredShader->setVec3("DirectionalLightColor", light.color);
                FDirectionalLightComponent::DirectionalLightDeferredShader->setVec3("cameraPos", GetWorldLocation());

                FDirectionalLightComponent::DirectionalLightDeferredShader->setInt("numOfCSM", light.numOfCSM);
                FDirectionalLightComponent::DirectionalLightDeferredShader->SetTexture("shadowMapCSM", light.shadowMap->Depth);
                for (int CSMIdx = 0; CSMIdx < light.numOfCSM; ++CSMIdx)
                {
                    FDirectionalLightComponent::DirectionalLightDeferredShader->setMat4(std::string("worldToShadowViewProj[") + std::to_string(CSMIdx) + "]", light.worldToShadowProj[CSMIdx]);
                }

                glDrawElements(GL_TRIANGLES, FDirectionalLightComponent::DirectionalLightDeferredGeo->GetNumOfIndices(), GL_UNSIGNED_INT, nullptr);

                glBindVertexArray(0);
                glUseProgram(0);
                

                //curFlipBufferIndex = lastFlipBuffer;
                break;
	            }
            case ELightType::LT_Point:
	            {
                uint8_t PointLightMask = 1 << 3;

                glEnable(GL_STENCIL_TEST);
                glStencilMask(PointLightMask);
                glStencilFunc(GL_NOTEQUAL, PointLightMask, PointLightMask);
                glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

                FPointLightComponent::PointLightDeferredShader->use();
                FPointLightComponent::PointLightDeferredGeo->use();

                FPointLightComponent::PointLightDeferredShader->SetTexture("gWorldPosMetallic", gBufferRef->Color[1]);
                FPointLightComponent::PointLightDeferredShader->SetTexture("gAlbedoSpec", gBufferRef->Color[2]);
                FPointLightComponent::PointLightDeferredShader->SetTexture("gWorldNormalRoughness", gBufferRef->Color[3]);
                FPointLightComponent::PointLightDeferredShader->setVec3("PointLightParams", light.direction);
                FPointLightComponent::PointLightDeferredShader->setVec3("PointLightColor", light.color);
                FPointLightComponent::PointLightDeferredShader->setVec3("PointLightPosition", light.location);
                FPointLightComponent::PointLightDeferredShader->setVec3("cameraPos", GetWorldLocation());

                FPointLightComponent::PointLightDeferredShader->setMat4("projection", GetView().project);
                FPointLightComponent::PointLightDeferredShader->setMat4("view", GetView().view);
                glm::mat4 matid(1);
                FPointLightComponent::PointLightDeferredShader->setMat4("model", glm::translate(matid, light.location)* glm::scale(matid, glm::vec3(light.radius / 0.5f)));

                glDrawArrays(GL_TRIANGLES, 0, FPointLightComponent::PointLightDeferredGeo->GetNumOfVertex());


                glClear(GL_STENCIL_BUFFER_BIT);

                glBindVertexArray(0);
                glUseProgram(0);
                glDisable(GL_STENCIL_TEST);
	            }

            	break;
            case ELightType::LT_Env:
            {
                    FEnvLightComponent::EnvLightDeferredShader->SetTexture("gEmissiveAO", gBufferRef->Color[0]);
                    FEnvLightComponent::EnvLightDeferredShader->SetTexture("gAlbedoSpec", gBufferRef->Color[2]);
                    FEnvLightComponent::EnvLightDeferredShader->setVec3("EnvLightColor", light.color);
                    FEnvLightComponent::EnvLightDeferredShader->use();
            		FEnvLightComponent::EnvLightDeferredGeo->use();

                    glDrawElements(GL_TRIANGLES, FEnvLightComponent::EnvLightDeferredGeo->GetNumOfIndices(), GL_UNSIGNED_INT, nullptr);

                    glBindVertexArray(0);
                    glUseProgram(0);
            }

                break;

            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        useFrameBuffer->Use();
        
        glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        if(!FinalShader)
        {
            FinalShader = std::make_shared<FShader>("./shaders/final_shader.vs", "./shaders/final_shader.fs");
        }
        if(!FinalPrimitive)
        {
            FinalPrimitive = std::make_shared<FPrimitive>();

            float geoDatas[] = {
                -1.0f, 1.0f, -0.5f,
                -1.0f, -1.0f,-0.5f,
                1.0f, -1.0f,-0.5f,
                1.0f, 1.0f, -0.5f
            };

            std::vector<char> vertex;
            vertex.resize(sizeof(geoDatas));
            memcpy(vertex.data(), geoDatas, vertex.size());

            std::vector<unsigned int> index = {
                0, 1, 2, 0, 2, 3
            };

            FPrimitiveVertexDesc desc;
            desc.structSize = 3 * sizeof(float);
            FPrimitiveVertexPropDesc prop(0, nullptr, GL_FLOAT, 3);
            desc.props.push_back(prop);

            FinalPrimitive->SetData(vertex, index, desc);
        }

        FinalShader->use();
        FinalPrimitive->use();

        FinalShader->SetTexture("sceneColor", gFlipBufferRefs[0]->Color[0]);

        glDrawElements(GL_TRIANGLES, FinalPrimitive->GetNumOfIndices(), GL_UNSIGNED_INT, nullptr);

        glBindVertexArray(0);
    	glUseProgram(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        gFlipBufferRefs[0]->Use();
        glViewport(0, 0, static_cast<GLsizei>(viewportSize.x), static_cast<GLsizei>(viewportSize.y));
        glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        for (auto&& renderBatch : renderBatches)
        {
            renderBatch.Shader->setVec3("DirectionalLightDir", glm::vec3(1, 0, 0));
            renderBatch.Shader->setVec3("DirectionalLightColor", glm::vec3(0, 0, 0));

            for (int pointLightId = 0; pointLightId < 4; ++pointLightId)
            {
                renderBatch.Shader->setVec4(std::string("PointLightLocationAndRadius[") + std::to_string(pointLightId) + "]", glm::vec4(0, 0, 0, 0));
                renderBatch.Shader->setVec3(std::string("PointLightColor[") + std::to_string(pointLightId) + "]", glm::vec3(0, 0, 0));
            }

            renderBatch.Shader->setVec3("EnvLightColor", glm::vec3(0, 0, 0));

            int pointLightNum = 0;
            for (auto&& light : lights)
            {
                switch (light.lightType)
                {
                case ELightType::LT_Directional:
                    renderBatch.Shader->setVec3("DirectionalLightDir", -light.direction);
                    renderBatch.Shader->setVec3("DirectionalLightColor", light.color);
                    break;
                case ELightType::LT_Point:
                    if (pointLightNum < 4)
                    {
                        renderBatch.Shader->setVec4(std::string("PointLightLocationAndRadius[") + std::to_string(pointLightNum) + "]", glm::vec4(light.location, light.radius));
                        renderBatch.Shader->setVec3(std::string("PointLightColor[") + std::to_string(pointLightNum) + "]", light.color);
                        ++pointLightNum;
                    }
                    break;
                case ELightType::LT_Env:
                    renderBatch.Shader->setVec3("EnvLightColor", light.color);
                    break;

                }
            }
            renderBatch.Draw(std::static_pointer_cast<FCameraComponent>(((FCameraComponent*)this)->GetObject()));
        }
        glUseProgram(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);


        useFrameBuffer->Use();

        glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        if (!FinalShader)
        {
            FinalShader = std::make_shared<FShader>("./shaders/final_shader.vs", "./shaders/final_shader.fs");
        }
        if (!FinalPrimitive)
        {
            FinalPrimitive = std::make_shared<FPrimitive>();

            float geoDatas[] = {
                -1.0f, 1.0f, -0.5f,
                -1.0f, -1.0f,-0.5f,
                1.0f, -1.0f,-0.5f,
                1.0f, 1.0f, -0.5f
            };

            std::vector<char> vertex;
            vertex.resize(sizeof(geoDatas));
            memcpy(vertex.data(), geoDatas, vertex.size());

            std::vector<unsigned int> index = {
                0, 1, 2, 0, 2, 3
            };

            FPrimitiveVertexDesc desc;
            desc.structSize = 3 * sizeof(float);
            FPrimitiveVertexPropDesc prop(0, nullptr, GL_FLOAT, 3);
            desc.props.push_back(prop);

            FinalPrimitive->SetData(vertex, index, desc);
        }

        FinalShader->use();
        FinalPrimitive->use();

        FinalShader->SetTexture("sceneColor", gFlipBufferRefs[0]->Color[0]);

        glDrawElements(GL_TRIANGLES, FinalPrimitive->GetNumOfIndices(), GL_UNSIGNED_INT, nullptr);

        glBindVertexArray(0);
        glUseProgram(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glEnable(GL_DEPTH_TEST);

    }
} 

std::shared_ptr<FTexture>& FTexture::GetBlack()
{
    static FTextureRef inner = std::make_shared<FTexture>(glm::vec3(0, 0, 0));
    return inner;
}

std::shared_ptr<FTexture>& FTexture::GetWhite()
{
    static FTextureRef inner = std::make_shared<FTexture>(glm::vec3(1, 1, 1));
    return inner;
}

const std::shared_ptr<FFrameBuffer>& FFrameBuffer::GetDefaultFrameBuffer()
{
    static std::shared_ptr<FFrameBuffer> inner = std::make_shared<FFrameBuffer>();
    return inner;
}


