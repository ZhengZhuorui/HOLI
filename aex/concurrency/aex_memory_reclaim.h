#pragma once
#include "tbb/combinable.h"
#include "tbb/enumerable_thread_specific.h"
#include "tbb/spin_mutex.h"
namespace aex{

enum class MemoryReclaimType{
    NodePtr,
    Memory,
    AtomicArray,
    HashNodeCopy,
    HashTable,
    HashTableBlock,
    Unknown
};

template<typename traits>
struct MemoryReclaimUnit{
    typedef aex_default_components<traits>     components;
    typedef typename components::node_ptr      node_ptr;
    typedef typename components::hash_node_ptr hash_node_ptr;
    typedef typename components::RWLock        RWLock;
    typedef typename components::HashTableBase HashTableBase;
    typedef typename components::slot_type     slot_type;
    typedef typename components::HashTableBlock HashTableBlock;

    //MemoryReclaimUnit(node_ptr node):pointer(node), type(MemoryReclaimType::NodePtr){}
    MemoryReclaimUnit() : type(MemoryReclaimType::Unknown) {}
    //MemoryReclaimUnit(MemoryReclaimType _type, void* mem)    :pointer(mem), type(_type){}
    MemoryReclaimUnit(MemoryReclaimType _type, node_ptr node) : pointer(node), type(MemoryReclaimType::NodePtr){
        AEX_ASSERT(_type == MemoryReclaimType::NodePtr);
    }
    MemoryReclaimUnit(MemoryReclaimType _type, hash_node_ptr node):pointer(node), type(MemoryReclaimType::HashNodeCopy){
        AEX_ASSERT(_type == MemoryReclaimType::HashNodeCopy);
    }
    MemoryReclaimUnit(MemoryReclaimType _type, HashTableBase* table_):pointer(table_), type(MemoryReclaimType::HashTable){
        AEX_ASSERT(_type == MemoryReclaimType::HashTable);
    }
    MemoryReclaimUnit(MemoryReclaimType _type, HashTableBlock* block):pointer(block), type(MemoryReclaimType::HashTableBlock){
        AEX_ASSERT(_type == MemoryReclaimType::HashTableBlock);
    }

    void* pointer;
    MemoryReclaimType type;
};

// Epoch based Memory Reclaim implementation is based on https://github.com/pohchaichon/ARTSynchronized
template<typename traits>
class aex_ThreadSpecificEpochBasedReclamationInformation {

    typedef aex_default_components<traits>     components;
    typedef aex_ThreadSpecificEpochBasedReclamationInformation<traits> self;
    typedef typename components::node_ptr      node_ptr;
    typedef typename components::hash_node_ptr hash_node_ptr;
    typedef typename components::RWLock        RWLock;
    typedef typename components::MRUnit        MRUnit;
    typedef typename components::Index         Index;
    typedef typename components::HashTable     HashTable;
    typedef typename components::HashTableBase     HashTableBase;
    typedef typename components::HashTableBlock HashTableBlock;
    typedef typename components::slot_type     slot_type;

    std::array<std::vector<MRUnit>, 3> mFreeLists;
    std::atomic <uint32_t> mLocalEpoch;
    uint32_t mPreviouslyAccessedEpoch;
    bool mThreadWantsToAdvance;
    //SALI<T, P> *tree;
    Index *index;

public:
    aex_ThreadSpecificEpochBasedReclamationInformation(Index* _index)
        : mFreeLists(), mLocalEpoch(3), mPreviouslyAccessedEpoch(3),
            mThreadWantsToAdvance(false), index(_index) {}

    aex_ThreadSpecificEpochBasedReclamationInformation(
        aex_ThreadSpecificEpochBasedReclamationInformation const &other) = delete;

    aex_ThreadSpecificEpochBasedReclamationInformation(
        aex_ThreadSpecificEpochBasedReclamationInformation &&other) = delete;

    ~aex_ThreadSpecificEpochBasedReclamationInformation() {
        for (uint32_t i = 0; i < 3; ++i) {
        freeForEpoch(i);
        }
    }

    void scheduleForDeletion(MRUnit unit) {
        assert(mLocalEpoch != 3);
        std::vector <MRUnit> &currentFreeList =
            mFreeLists[mLocalEpoch];
        currentFreeList.emplace_back(unit);
        mThreadWantsToAdvance = (currentFreeList.size() % 64u) == 0;
    }

    uint32_t getLocalEpoch() const {
        return mLocalEpoch.load(std::memory_order_acquire);
    }

    void enter(uint32_t newEpoch) {
        assert(mLocalEpoch == 3);
        if (mPreviouslyAccessedEpoch != newEpoch) {
        freeForEpoch(newEpoch);
        mThreadWantsToAdvance = false;
        mPreviouslyAccessedEpoch = newEpoch;
        }
        mLocalEpoch.store(newEpoch, std::memory_order_release);
    }

    void leave() { mLocalEpoch.store(3, std::memory_order_release); }

    bool doesThreadWantToAdvanceEpoch() { return (mThreadWantsToAdvance); }

private:
    void freeForEpoch(uint32_t epoch) {
        std::vector < MRUnit > &previousFreeList =
            mFreeLists[epoch];
        //AEX_WARNING("memory reclaim");

        for (MRUnit unit: previousFreeList) {
            switch (unit.type){
                case MemoryReclaimType::Memory:{
                    //AEX_WARNING("memory reclaim");
                    free(unit.pointer); break;
                }
                case MemoryReclaimType::NodePtr:{
                    //AEX_WARNING("node reclaim");
                    index->free_node(n_n(unit.pointer)); 
                    break;
                }
                case MemoryReclaimType::HashNodeCopy:{
                    //AEX_WARNING("HashNodeCopy reclaim");
                    index->clear_copy(h_n(unit.pointer)); break;
                }
                case MemoryReclaimType::HashTable:{
                    //AEX_WARNING("HashTable reclaim");
                    reinterpret_cast<HashTableBase*>(unit.pointer)->free_hash_table();
                    delete reinterpret_cast<HashTableBase*>(unit.pointer);
                    break;
                }
                case MemoryReclaimType::HashTableBlock:{
                    delete reinterpret_cast<HashTableBlock*>(unit.pointer);
                    break;
                }
                default:{
                    AEX_ERROR("Unknown Type");
                    AEX_ASSERT(0 == 1);
                    break;
                }
            }
        }
        previousFreeList.resize(0u);
    }
};


template<typename traits>
class aex_EpochBasedMemoryReclamationStrategy {
public:

    typedef aex_default_components<traits>     components;
    typedef typename components::node_ptr      node_ptr;
    typedef typename components::hash_node_ptr hash_node_ptr;
    typedef typename components::RWLock        RWLock;
    typedef typename components::MRUnit        MRUnit;
    typedef typename components::Index         Index;
    typedef aex_EpochBasedMemoryReclamationStrategy<traits> self;
    typedef typename components::ThreadSpecificEpochBasedReclamationInformation ThreadSpecificEpochBasedReclamationInformation;
    typedef typename components::HashTable     HashTable;
    typedef typename components::slot_type     slot_type;

    uint32_t NEXT_EPOCH[3] = {1, 2, 0};
    uint32_t PREVIOUS_EPOCH[3] = {2, 0, 1};

    std::atomic <uint32_t> mCurrentEpoch;
    Index *index;
    tbb::enumerable_thread_specific <
    ThreadSpecificEpochBasedReclamationInformation,
    tbb::cache_aligned_allocator<ThreadSpecificEpochBasedReclamationInformation>,
    tbb::ets_key_per_instance>
        mThreadSpecificInformations;

    aex_EpochBasedMemoryReclamationStrategy(Index *_index)
        : mCurrentEpoch(0), index(_index), mThreadSpecificInformations(index) {}
    ~aex_EpochBasedMemoryReclamationStrategy(){}

public:
    static self *getInstance(const Index *index) {
        return index->ebr;
    }

    void enterCriticalSection() {
        ThreadSpecificEpochBasedReclamationInformation &currentMemoryInformation =
            mThreadSpecificInformations.local();
        uint32_t currentEpoch = mCurrentEpoch.load(std::memory_order_acquire);
        currentMemoryInformation.enter(currentEpoch);
        if (currentMemoryInformation.doesThreadWantToAdvanceEpoch() && canAdvance(currentEpoch)) {
            mCurrentEpoch.compare_exchange_strong(currentEpoch, NEXT_EPOCH[currentEpoch]);
        }
    }

    bool canAdvance(uint32_t currentEpoch) {
        uint32_t previousEpoch = PREVIOUS_EPOCH[currentEpoch];
        return !std::any_of(
            mThreadSpecificInformations.begin(),
            mThreadSpecificInformations.end(),
            [previousEpoch](ThreadSpecificEpochBasedReclamationInformation const
                            &threadInformation) {
            return (threadInformation.getLocalEpoch() == previousEpoch);
            });
    }

    void leaveCriticialSection() {
        ThreadSpecificEpochBasedReclamationInformation &currentMemoryInformation =
            mThreadSpecificInformations.local();
        currentMemoryInformation.leave();
    }

    void scheduleForDeletion(MRUnit unit) {
        mThreadSpecificInformations.local().scheduleForDeletion(unit);
    }
};

template<typename traits>
class aex_EpochGuard {
    typedef aex_default_components<traits> components;
    typedef typename components::Index Index;
    typedef typename components::EpochBasedMemoryReclamationStrategy EpochBasedMemoryReclamationStrategy;
    EpochBasedMemoryReclamationStrategy *instance;

public:
    aex_EpochGuard(const Index *index) {
        instance = EpochBasedMemoryReclamationStrategy::getInstance(index);
        instance->enterCriticalSection();
    }

    ~aex_EpochGuard() { instance->leaveCriticialSection(); }
};
}