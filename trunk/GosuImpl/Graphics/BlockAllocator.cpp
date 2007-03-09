#include <GosuImpl/Graphics/BlockAllocator.hpp>
#include <stdexcept>
#include <vector>

struct Gosu::BlockAllocator::Impl
{
    unsigned width, height;

    typedef std::vector<Block> Blocks;
    Blocks blocks;
    unsigned firstX, firstY;
    unsigned maxW, maxH;

    bool isBlockFree(const Block& block) const
    {
        // (The right-th column and the bottom-th row are outside of the block.)
        unsigned right = block.left + block.width;
        unsigned bottom = block.top + block.height;

        // Block isn't valid.
        if (right > width || bottom > height)
            return false;

        // Test if the block collides with any existing rects.
        Blocks::const_iterator i, end = blocks.end();
        for (i = blocks.begin(); i != end; ++i)
            if (i->left < right && block.left < i->left + i->width &&
                i->top < bottom && block.top < i->top + i->height)
            {
                return false;
            }

        return true;
    }
};

Gosu::BlockAllocator::BlockAllocator(unsigned width, unsigned height)
: pimpl(new Impl)
{
    pimpl->width = width;
    pimpl->height = height;

    pimpl->firstX = 0;
    pimpl->firstY = 0;

    pimpl->maxW = width;
    pimpl->maxH = height;
}

Gosu::BlockAllocator::~BlockAllocator()
{
}

unsigned Gosu::BlockAllocator::width() const
{
    return pimpl->width;
}

unsigned Gosu::BlockAllocator::height() const
{
    return pimpl->height;
}

boost::optional<Gosu::BlockAllocator::Block>
    Gosu::BlockAllocator::alloc(unsigned aWidth, unsigned aHeight)
{
    // The rect wouldn't even fit onto the texture!
    if (aWidth > width() || aHeight > height())
        return boost::optional<Block>();

    // It could theoretically fit, but there's no space left.
    if (aWidth > pimpl->maxW && aHeight > pimpl->maxH)
        return boost::optional<Block>();
    
    // Start to look for a place next to the last returned rect. Chances are
    // good we'll find a place there.
    Block b = Block(pimpl->firstX, pimpl->firstY, aWidth, aHeight);
    if (pimpl->isBlockFree(b))
    {
        pimpl->blocks.push_back(b);
        return b;
    }

    // Brute force: Look for a free place on this texture.
    unsigned& x = b.left;
    unsigned& y = b.top;
    for (y = 0; y <= height() - aHeight; y += 16)
        for (x = 0; x <= width() - aWidth; x += 8)
        {
            if (!pimpl->isBlockFree(b))
                continue;

            // Found a nice place!

            // Try to make up for the large for()-stepping.
            while (y > 0 && pimpl->isBlockFree(Block(x, y - 1, aWidth, aHeight)))
                --y;
            while (x > 0 && pimpl->isBlockFree(Block(x - 1, y, aWidth, aHeight)))
                --x;

            pimpl->blocks.push_back(b);
            return b;
        }

    // So there was no space for the bitmap. Remember this for later.
    pimpl->maxW = aWidth - 1;
    pimpl->maxH = aHeight - 1;
    return boost::optional<Block>();
}

void Gosu::BlockAllocator::free(unsigned left, unsigned top)
{
    for (Impl::Blocks::iterator i = pimpl->blocks.begin();
        i != pimpl->blocks.end(); ++i)
    {
        if (i->left == left && i->top == top)
        {
            // Since we have freed up some space, it might make sense to look
            // for textures up to our size again.
            if (i->left >= pimpl->maxW && i->top >= pimpl->maxH)
            {
                // Be really optimistic about the new space.
                pimpl->maxW += i->left;
                if (pimpl->maxW > width())
                    pimpl->maxW = width();
                pimpl->maxH += i->top;
                if (pimpl->maxH > height())
                    pimpl->maxH = height();
            }

            pimpl->blocks.erase(i);
            return;
        }
    }

    throw std::logic_error("Tried to free an invalid block");
}
