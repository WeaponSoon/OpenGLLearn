#pragma once
#include <unordered_set>
#include <glm/glm.hpp>
#include "core_m2.h"
#include "scene_m2.h"
#include "scene_m2.h"


struct BVHBound
{
	glm::vec3 begin = glm::vec3(0);
	glm::vec3 end = glm::vec3(0);
	glm::vec3 Center() const
	{
		return (begin + end) * glm::vec3(0.5f, 0.5f, 0.5f);
	}
};

struct BVHTreeWeakPtrCanAsKey
{
private:
	class BVHTree* TreeHasher = nullptr;
	std::weak_ptr<class BVHTree> TreePtr;
public:
	size_t GetHash() const
	{
		return std::hash<void*>()(TreeHasher);
	}

	static BVHTreeWeakPtrCanAsKey BuildKeyForSearch(class BVHTree* InTree)
	{
		BVHTreeWeakPtrCanAsKey Ret;
		Ret.TreeHasher = InTree;
		return Ret;
	}

	BVHTreeWeakPtrCanAsKey() = default;
	BVHTreeWeakPtrCanAsKey(const BVHTreeWeakPtrCanAsKey& InOther) = default;

	BVHTreeWeakPtrCanAsKey(BVHTree* InTree);

	bool IsValid() const;

	bool operator==(const BVHTree* InTree) const;
	bool operator==(const std::shared_ptr<BVHTree>& InTree) const;
	bool operator==(const BVHTreeWeakPtrCanAsKey& InOther) const;
	class BVHTree* operator->() const;

	BVHTreeWeakPtrCanAsKey& operator=(const BVHTreeWeakPtrCanAsKey& InOther);
	BVHTreeWeakPtrCanAsKey& operator=(BVHTree* InTree);
	BVHTreeWeakPtrCanAsKey& operator=(const std::shared_ptr<BVHTree>& InTree);
};

struct BVHTreeKey
{
	size_t Index = 0;
	BVHTreeWeakPtrCanAsKey Tree = nullptr;
	class BVHTreeNode* GetNode() const;
	void Refresh() const;
	bool operator==(const BVHTreeKey& Other) const { return Tree == Other.Tree && Index == Other.Index; }
};
class BVHTree;



namespace std
{
	template<>
	struct hash<BVHTreeWeakPtrCanAsKey>
	{
		size_t operator()(const BVHTreeWeakPtrCanAsKey& v) const noexcept
		{
			return v.GetHash();
		}
	};

	template<>
	struct hash<BVHTreeKey>
	{
		inline void CombineHash(size_t& Seed, size_t Hash) const noexcept
		{
			Seed ^= Hash + 0x9e3779b9 + (Seed << 6) + (Seed >> 2);
		}
		size_t operator()(const BVHTreeKey& v) const noexcept
		{
			size_t HashIndex = hash<size_t>()(v.Index);
			size_t HashTree = hash<BVHTreeWeakPtrCanAsKey>()(v.Tree);
			CombineHash(HashIndex, HashTree);
			return HashIndex;
		}
	};
}



class  IBVHTreeItem : virtual public FVeryBase
{
public:

	virtual std::unordered_set<BVHTreeKey>& GetAABBTreeKeys() = 0;
	virtual const BVHBound& GetBox() const = 0;
	virtual bool ShouldAdd() const = 0;

	std::shared_ptr<IBVHTreeItem> GetBVHItem() const
	{
		return std::dynamic_pointer_cast<IBVHTreeItem>(std::const_pointer_cast<FVeryBase>(shared_from_this()));
	}

	virtual ~IBVHTreeItem() = default;
};



class  BVHTreeNode
{
	friend class BVHTree;

	//#if WITH_EDITOR
	//	public:
	//#endif

	size_t MyIndex = 0;

	std::weak_ptr<IBVHTreeItem> Block;

	glm::vec3 Min;
	glm::vec3 Max;

	class BVHTree* Tree = nullptr;

	size_t Parent = 0;
	size_t LeftChild = 0;
	size_t RightChild = 0;

	BVHTreeNode* GetParent() const;
	BVHTreeNode* GetLeft() const;
	BVHTreeNode* GetRight() const;

	//KFAABBTreeNode* Parent = nullptr;
	//KFAABBTreeNode* LeftChild = nullptr;
	//KFAABBTreeNode* RightChild = nullptr;

	void TryExpand(const BVHTreeNode& InChild);

	void Reset()
	{
		Min = glm::vec3(0, 0, 0);
		Max = Min;
	}



public:
	BVHTreeNode() = default;
	BVHTreeNode(const BVHTreeNode&) = default;
	BVHTreeNode(BVHTreeNode&&) = default;

	BVHTreeNode& operator=(const BVHTreeNode&) = default;

	static void DestructNode(const BVHTreeKey& Node);


	bool Contains(const BVHTreeNode& InOther) const;
	bool Overlap(const BVHTreeNode& InOther) const;
	bool IsValid() const
	{
		return Min.x < Max.x;
	}

	float Volume() const
	{
		glm::vec3 Size = Max - Min;
		return Size.x * Size.y * Size.z;
	}

	glm::vec3 Center() const;
};

class  BVHTree : public std::enable_shared_from_this<BVHTree>
{
	friend class BVHTreeNode;
	friend struct BVHTreeKey;
	//#if WITH_EDITOR
	//	public:
	//#endif

	std::vector<BVHTreeNode> Nodes;
	std::vector<size_t> UnusedNodeIndex;
	//KFAABBTreeNode* Root = nullptr;

	/*这些函数先简单的调用new delete copy等，后续AABBTree要有自己的内存管理*/

	BVHTreeNode* GetRoot() { return Nodes.size() > 1 ? &Nodes[1] : nullptr; }

	const BVHTreeNode* GetRoot() const { return Nodes.size() > 1 ? &Nodes[1] : nullptr; }

	const size_t GetRootIndex() const { return 1; }

	//new 
	size_t NewNode();
	//delete
	static void DeleteNode(BVHTreeNode* InNode);

	//copy construct
	void CopyConstructNode(BVHTreeNode* Target, BVHTreeNode* Source)
	{
		new (Target) BVHTreeNode(*Source);
		Target->Tree = this;
	}
	//operator=
	void AssignNode(BVHTreeNode* Target, BVHTreeNode* Source)
	{
		size_t TargetIndex = Target->MyIndex;
		*Target = *Source;
		Target->Tree = this;
		Target->MyIndex = TargetIndex;
	}

	void RefreshNode(size_t TargetIndex);

	void FindPosition(size_t InParent, size_t InNewNode);

	void DeleteTree(BVHTreeNode* InNode);

	void Remove(BVHTreeNode* Node, bool bDeleteNode = true);

	size_t Build(std::vector<std::weak_ptr<IBVHTreeItem>>::iterator Begin, std::vector<std::weak_ptr<IBVHTreeItem>>::iterator End, bool bDontSliceByZ);

	void ExchangeNodeTree(BVHTreeNode* Node);

	void GetBlocksMayOverlap(glm::vec3 InMin, glm::vec3 InMax, const BVHTreeNode* InParent, std::vector<std::weak_ptr<IBVHTreeItem>>& Out) const;

	void ConstructByBlock(size_t Index, std::weak_ptr<IBVHTreeItem> InBlock);


	BVHTree() = default;
	BVHTree(const BVHTree&);

public:

	void DrawDebug(glm::mat4 v, glm::mat4 p, glm::vec3 cameraPos);

	class KFAABBTreeSharedPtrParam
	{
		friend class BVHTree;

	private:
		KFAABBTreeSharedPtrParam() {};
	};

	BVHTree(const KFAABBTreeSharedPtrParam&) : BVHTree() {};

	static std::shared_ptr<BVHTree> CreateTree();


	std::vector<std::weak_ptr<IBVHTreeItem>> GetBlocksMayOverlap(glm::vec3 InMin, glm::vec3 InMax) const;


	BVHTree& operator=(const BVHTree&); //cannot copy




	void Insert(std::weak_ptr<IBVHTreeItem> NewBlock);
	void Remove(std::weak_ptr<IBVHTreeItem> InBlock);
	void Build(std::vector<std::weak_ptr<IBVHTreeItem>>& Blocks, bool bDontSliceByZ = false);


	void Destroy();
	~BVHTree()
	{
		Destroy();
	}

};

inline BVHTreeWeakPtrCanAsKey::BVHTreeWeakPtrCanAsKey(BVHTree* InTree)
{
	if (InTree)
	{
		TreePtr = InTree->shared_from_this();
		TreeHasher = InTree;
	}
	else
	{
		TreePtr.reset();
		TreeHasher = nullptr;
	}
}

inline bool BVHTreeWeakPtrCanAsKey::IsValid() const
{
	auto&& SharedPtr = TreePtr.lock();
	if (SharedPtr)
	{
		return true;
	}
	return false;
}

inline bool BVHTreeWeakPtrCanAsKey::operator==(const BVHTree* InTree) const
{
	return InTree == TreeHasher;
}

inline bool BVHTreeWeakPtrCanAsKey::operator==(const std::shared_ptr<BVHTree>& InTree) const
{
	return TreePtr.lock() == InTree;
}

inline bool BVHTreeWeakPtrCanAsKey::operator==(const BVHTreeWeakPtrCanAsKey& InOther) const
{
	return TreeHasher == InOther.TreeHasher;
}

inline BVHTree* BVHTreeWeakPtrCanAsKey::operator->() const
{
	auto&& SharedTree = TreePtr.lock();
	//kfAssert(SharedTree && SharedTree.get() == TreeHasher);
	return SharedTree.get();
}

inline BVHTreeWeakPtrCanAsKey& BVHTreeWeakPtrCanAsKey::operator=(const BVHTreeWeakPtrCanAsKey& InOther)
{
	TreeHasher = InOther.TreeHasher;
	TreePtr = InOther.TreePtr;
	return *this;
}

inline BVHTreeWeakPtrCanAsKey& BVHTreeWeakPtrCanAsKey::operator=(BVHTree* InTree)
{
	if (InTree)
	{
		TreePtr = InTree->shared_from_this();
		TreeHasher = InTree;
	}
	else
	{
		TreePtr.reset();
		TreeHasher = nullptr;
	}
	return *this;
}

inline BVHTreeWeakPtrCanAsKey& BVHTreeWeakPtrCanAsKey::operator=(const std::shared_ptr<BVHTree>& InTree)
{
	TreePtr = InTree;
	TreeHasher = InTree.get();
	return *this;
}

inline BVHTreeNode* BVHTreeKey::GetNode() const
{
	return &Tree->Nodes[Index];
}

inline void BVHTreeKey::Refresh() const
{
	Tree->RefreshNode(Index);
}

inline BVHTreeNode* BVHTreeNode::GetParent() const
{
	if (Tree && Parent)
	{
		return &Tree->Nodes[Parent];
	}
	return nullptr;
}

inline BVHTreeNode* BVHTreeNode::GetLeft() const
{
	if (Tree && LeftChild)
	{
		return &Tree->Nodes[LeftChild];
	}
	return nullptr;
}

inline BVHTreeNode* BVHTreeNode::GetRight() const
{
	if (Tree && RightChild)
	{
		return &Tree->Nodes[RightChild];
	}
	return nullptr;
}


enum class EAttachRule
{
    AR_KeepWorld,
    AR_KeepRelative,
    AR_SnapToTarget,
};

class FSceneComponent : public FComponent, public IBVHTreeItem
{
    glm::mat4 matrix = glm::mat4(1.0f);
    std::weak_ptr<FSceneComponent> parent;
    std::set<std::shared_ptr<FSceneComponent>> children;

    bool HasChild(const std::shared_ptr<FSceneComponent>& inChild) const
    {
        if (children.find(inChild) != children.end())
        {
            return true;
        }

        for (auto&& child : children)
        {
            if (child->HasChild(inChild))
            {
                return true;
            }
        }

        return false;
    }

	std::unordered_set<BVHTreeKey> Keys;
	BVHBound BoundCache;


	


public:
	void UpdateBoundCache();

	virtual void OnUpdateBoundCache()
	{
		auto WorldLocation = GetWorldLocation();

		BoundCache.begin = WorldLocation - glm::vec3(0.1f, 0.1f, 0.1f);
		BoundCache.end = WorldLocation + glm::vec3(0.1f, 0.1f, 0.1f);
	}

	std::unordered_set<BVHTreeKey>& GetAABBTreeKeys() override { return Keys; }
	const BVHBound& GetBox() const override { return BoundCache; }
	bool ShouldAdd() const override { return true; }
	virtual void FinalTick(float deltaSecond) override;

    std::weak_ptr<FSceneComponent> GetParent() const
    {
        return parent;
    }

    std::set<std::shared_ptr<FSceneComponent>> GetAllChildren() const
    {
        return children;
    }

    glm::mat4 GetWorldTransform() const
    {
        std::shared_ptr<FSceneComponent> safe_parent = parent.lock();
        if (safe_parent)
        {
            return safe_parent->GetWorldTransform() * matrix;
        }
        return matrix;
    }

    glm::mat4 GetLocalTransform() const
    {
        return matrix;
    }

    void SetLocalTransform(const glm::mat4& inTransform)
    {
        matrix = inTransform;
		UpdateBoundCache();
    }

    void SetWorldTransform(const glm::mat4& inTransform)
    {
        std::shared_ptr<FSceneComponent> safe_parent = parent.lock();
        if (!safe_parent)
        {
            matrix = inTransform;
        }
        else
        {
            auto&& parentTransform = safe_parent->GetWorldTransform();
            matrix = glm::inverse(parentTransform) * inTransform;
        }
		UpdateBoundCache();
    }

    glm::vec3 GetLocalLocation() const
    {
        return (matrix[3]);
    }

    glm::vec3 GetWorldLocation() const
    {
        return GetWorldTransform()[3];
    }

    void SetLocalLocation(const glm::vec3& inLocation)
    {
        matrix[3] = glm::vec4(inLocation, matrix[3][3]);
		UpdateBoundCache();
    }

    void SetWorldLocation(const glm::vec3& inLocation)
    {
        auto worldMatrix = GetWorldTransform();
        worldMatrix[3] = glm::vec4(inLocation, worldMatrix[3][3]);
        SetWorldTransform(worldMatrix);
    }

    glm::vec3 GetFowardInWorldSpace() const
    {
        return normalize((GetWorldTransform() * glm::vec4(0, 0, -1, 0)));
    }

    glm::vec3 GetRightInWorldSpace() const
    {
        return normalize((GetWorldTransform() * glm::vec4(1, 0, 0, 0)));
    }

    glm::vec3 GetUpInWorldSpace() const
    {
        return normalize((GetWorldTransform() * glm::vec4(0, 1, 0, 0)));
    }

    void AttachTo(std::shared_ptr<FSceneComponent> newParent, EAttachRule inRule)
    {
        auto shared_this = std::static_pointer_cast<FSceneComponent>(this->GetObject());

        if (newParent == shared_this)
        {
            return;
        }

        std::shared_ptr<FSceneComponent> safe_parent = parent.lock();
        if (safe_parent == newParent)
        {
            return;
        }

        if (HasChild(newParent))
        {
            return;
        }

        glm::mat4 oldWorld = GetWorldTransform();
        if (safe_parent)
        {
            safe_parent->children.erase(shared_this);
        }
        parent = newParent;
        switch (inRule)
        {
        case EAttachRule::AR_KeepRelative:
            break;
        case EAttachRule::AR_SnapToTarget:
            SetLocalTransform(glm::mat4(1.0f));
            break;
        case EAttachRule::AR_KeepWorld:
            SetWorldTransform(oldWorld);
            break;
        }
		UpdateBoundCache();
    }
};


class FScene : public FObject
{
    std::vector<std::shared_ptr<FComponent>> components;
	std::shared_ptr<BVHTree> Tree = BVHTree::CreateTree();
public:

	void DrawDebug(glm::mat4 v, glm::mat4 p, glm::vec3 cp);

	void UpdateBVH(const std::shared_ptr<IBVHTreeItem>& item)
	{
		auto&& keys = item->GetAABBTreeKeys();
		if(keys.empty())
		{
			Tree->Insert(item);
		}
		for(auto&& key : keys)
		{
			key.Refresh();
		}
	}

    template<typename T>
	typename std::enable_if<HasInit<T, void>::value, std::shared_ptr<T>>::type CreateComponent()
    {
        auto ret = std::make_shared<T>();
        auto base = std::static_pointer_cast<FComponent>(ret);
        base->scene = std::static_pointer_cast<FScene>(this->GetObject());
        components.push_back(base);
		ret->Init();
        return ret;
    }

	template<typename T>
	typename std::enable_if<!HasInit<T, void>::value, std::shared_ptr<T>>::type CreateComponent()
	{
		auto ret = std::make_shared<T>();
		auto base = std::static_pointer_cast<FComponent>(ret);
		base->scene = std::static_pointer_cast<FScene>(this->GetObject());
		components.push_back(base);
		return ret;
	}

    template<typename T, typename...Args>
    typename std::enable_if<HasInit<T, void, Args...>::value, std::shared_ptr<T>>::type CreateComponentWithArg(Args...args)
    {
        auto ret = std::make_shared<T>();
        auto base = std::static_pointer_cast<FComponent>(ret);
        base->scene = std::static_pointer_cast<FScene>(this->GetObject());
        components.push_back(base);
		ret->Init(std::forward<Args>(args)...);
        return ret;
    }

	template<typename T, typename...Args>
	typename std::enable_if<!HasInit<T, void, Args...>::value, std::shared_ptr<T>>::type CreateComponentWithArg(Args...args)
	{
		auto ret = std::make_shared<T>();
		auto base = std::static_pointer_cast<FComponent>(ret);
		base->scene = std::static_pointer_cast<FScene>(this->GetObject());
		components.push_back(base);
		return ret;
	}

    const std::vector<std::shared_ptr<FComponent>>& GetAllComponents() const
    {
        return components;
    }

    void Tick(float deltaSecond)
    {
        auto componentsCopy = components;

        for (auto&& component : componentsCopy)
        {
            component->PreTick(deltaSecond);
        }

        for (auto&& component : componentsCopy)
        {
            component->EarlyTick(deltaSecond);
        }

        for (auto&& component : componentsCopy)
        {
            component->Tick(deltaSecond);
        }

        for (auto&& component : componentsCopy)
        {
            component->PostTick(deltaSecond);
        }

        for (auto&& component : componentsCopy)
        {
            component->FinalTick(deltaSecond);
        }
    }

};

using FSceneRef = std::shared_ptr<FScene>;

