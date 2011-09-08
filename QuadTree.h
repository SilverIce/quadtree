#pragma once
#include <memory.h>

typedef unsigned int uint32;
typedef unsigned long long uint64;
typedef unsigned char uint8;

typedef uint32 length_type;

struct Point
{
    length_type x, y;
};

struct Circle
{
    Point point;
    length_type radius;
};

struct AABox2d
{
    Point lo;
    Point hi;

    static AABox2d create(const Circle& c)
    {
        AABox2d box = {
            {c.point.x-c.radius, c.point.y-c.radius},
            {c.point.x+c.radius, c.point.y+c.radius}
        };
        return box;
    }
};

enum ChildOffset{
    LeftUpper  = 0,
    RightUpper = 1,
    LeftLower  = 2,
    RightLower = 3,
};

enum QuadrantFlags{
    NorthWest  = 1 << LeftUpper,
    NorthEast  = 1 << RightUpper,
    SouthWest  = 1 << LeftLower,
    SouthEast  = 1 << RightLower,
};

// divides space into four subspaces
struct SpaceDivision 
{
    length_type xDiv, yDiv;

    enum IntersectionResult{
        None        = 0x0,
        Left        = 0x1,
        Right       = 0x2,
        Lower       = 0x4,
        Upper       = 0x8,

        Mask_LeftUpper   = Left|Upper,   // 9
        Mask_RightUpper  = Right|Upper,  // 10
        Mask_LeftLower   = Left|Lower,   // 5
        Mask_RightLower  = Right|Lower,  // 6
    };

    /*IntersectionResult intersection(const Point& p) const
    {
        IntersectionResult result = (p.x >= xDiv) ? Right : Left;
        result |= (p.y >= yDiv) ? Upper : Lower;
        return result;
    }*/

    IntersectionResult intersection(const Circle& p) const
    {
        return intersection(AABox2d::create(p));
    }

    IntersectionResult intersection(const AABox2d& p) const
    {
        uint8 result(None);

        if (xDiv < p.lo.x)
            result |= Right;
        else if (xDiv > p.hi.x)
            result |= Left;
        else
            result |= (Left|Right);

        if (yDiv < p.lo.y)
            result |= Upper;
        else if (yDiv > p.hi.y)
            result |= Lower;
        else
            result |= (Lower|Upper);
        return (IntersectionResult)result;
    }

    QuadrantFlags intersectionQuadrants(const AABox2d& p) const
    {
        if (xDiv < p.lo.x) //right
        {
            if (yDiv < p.lo.y)
                return NorthEast;
            else if (yDiv > p.hi.y)
                return SouthEast;
            else
                return (QuadrantFlags)(NorthEast|SouthEast);
        }
        else if (xDiv > p.hi.x) // left
        {
            if (yDiv < p.lo.y)
                return NorthWest;
            else if (yDiv > p.hi.y)
                return SouthWest;
            else
                return (QuadrantFlags)(NorthWest|SouthWest);
        }
        else //right | left
        {
            if (yDiv < p.lo.y)
                return (QuadrantFlags)(NorthEast|NorthWest);
            else if (yDiv > p.hi.y)
                return (QuadrantFlags)(SouthEast|SouthWest);
            else
                return (QuadrantFlags)(SouthEast|SouthWest | NorthEast|NorthWest);
        }
    }
};

class QuadTree
{
public:

    // calculates total amount of nodes for given tree depth
    static uint64 NodesAmount(uint8 depth)
    {
        return ((uint64(1) << (2*depth+2)) - 1) / 3;
    }

    // calculates amount of nodes for given level(depth)
    static uint64 NodesPerLevelAmount(uint8 depth)
    {
        return (uint64(1) << (2*depth));
    }

    typedef SpaceDivision Node;

    enum{
        DBG_WORD = 0,
        //MaxUInt32Depth = 14,
        //MaxUInt64Depth = 30,
    };

    static QuadTree* create(uint8 Depth, const Point& center, length_type sideSize)
    {
        uint64 nodesAmount = NodesAmount(Depth);
        if (nodesAmount > ((uint64(1) << 32) - 1))  // nodes amount is greater than maximum unsigned 32-bit value
            return 0;

        QuadTree * tree = new QuadTree();
        tree->m_depth = Depth;
        tree->nodes = new Node[(uint32)nodesAmount];
        tree->initNodes(center, sideSize, (uint32)nodesAmount);
        return tree;
    }

    ~QuadTree() { delete[] nodes;}

    Node * Nodes() const { return nodes;}
    uint8 Depth() const { return m_depth;}

    void debugSelf();

    template<class T> void intersect(const AABox2d& p, T& visitor) const;

    template<class T> void intersectRecursive(const AABox2d& p, T& visitor) const;

    struct QuadIterator deepestContaining(const AABox2d& p) const;

private:
    QuadTree() {}
    QuadTree(const QuadTree&);
    QuadTree& operator = (const QuadTree&);
    void initNodes(const Point& center, length_type sideSize, uint32 nodesAmount);

    Node * nodes;
    uint8 m_depth;
};

struct QuadIterator
{
    QuadTree::Node * my_table;
    uint32 my_adress;
    uint8 myDepth;
    uint8 lastDepth;

    static QuadIterator create(const QuadTree * tree)
    {
        QuadIterator it = {tree->Nodes(), (uint32)0, (uint8)0, tree->Depth()};
        return it;
    }

    bool moveNext()
    {
        if (myDepth == lastDepth)
            return false;

        my_table += QuadTree::NodesPerLevelAmount(myDepth);
        ++myDepth;
        my_adress *= 4;
        return true;
    }

    bool movePrev()
    {
        if (myDepth == 0)
            return false;

        my_adress /= 4;
        --myDepth;
        my_table -= QuadTree::NodesPerLevelAmount(myDepth);
        return true;
    }

    QuadIterator nearby(ChildOffset offset_) const
    {
        QuadIterator it = *this;
        it.moveTo(offset_);
        return it;
    }

    void moveTo(ChildOffset offset_)
    {
        my_adress = my_adress/4*4 + offset_;
    }

    ChildOffset offset() const
    {
        return (ChildOffset)(my_adress % 4);
    }

    QuadTree::Node* current() const { return my_table + my_adress;}
};



#include "QuadTree.inl"
