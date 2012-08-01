#ifndef __TRANSACTIONMETA__
#define __TRANSACTIONMETA__

#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include "../json/value.h"

#include "uint256.h"
#include "Serializer.h"
#include "SerializedTypes.h"


class TransactionMetaNodeEntry
{ // a way that a transaction has affected a node
public:

	typedef boost::shared_ptr<TransactionMetaNodeEntry> pointer;

	static const int TMNEndOfMetadata =	0;
	static const int TMNChangedBalance = 1;
	static const int TMNDeleteUnfunded = 2;

	int mType;
	TransactionMetaNodeEntry(int type) : mType(type) { ; }

	int getType() const { return mType; }
	virtual Json::Value getJson(int) const = 0;
	virtual void addRaw(Serializer&) const = 0;
	virtual int compare(const TransactionMetaNodeEntry&) const = 0;

	bool operator<(const TransactionMetaNodeEntry&) const;
	bool operator<=(const TransactionMetaNodeEntry&) const;
	bool operator>(const TransactionMetaNodeEntry&) const;
	bool operator>=(const TransactionMetaNodeEntry&) const;

	virtual std::auto_ptr<TransactionMetaNodeEntry> clone() const
	{ return std::auto_ptr<TransactionMetaNodeEntry>(duplicate()); }

protected:
	virtual TransactionMetaNodeEntry* duplicate(void) const = 0;
};

class TMNEBalance : public TransactionMetaNodeEntry
{ // a transaction affected the balance of a node
public:

	static const int TMBTwoAmounts 	= 0x001;
	static const int TMBDestroyed	= 0x010;
	static const int TMBPaidFee		= 0x020;
	static const int TMBRipple		= 0x100;
	static const int TMBOffer		= 0x200;

protected:
	unsigned mFlags;
	STAmount mFirstAmount, mSecondAmount;

public:
	TMNEBalance() : TransactionMetaNodeEntry(TMNChangedBalance), mFlags(0) { ; }

	TMNEBalance(SerializerIterator&);
	virtual void addRaw(Serializer&) const;

	unsigned getFlags() const				{ return mFlags; }
	const STAmount& getFirstAmount() const	{ return mFirstAmount; }
	const STAmount& getSecondAmount() const	{ return mSecondAmount; }

	void adjustFirstAmount(const STAmount&);
	void adjustSecondAmount(const STAmount&);
	void setFlags(unsigned flags);

	virtual Json::Value getJson(int) const;
	virtual int compare(const TransactionMetaNodeEntry&) const;
	virtual TransactionMetaNodeEntry* duplicate(void) const { return new TMNEBalance(*this); }
};

class TMNEUnfunded : public TransactionMetaNodeEntry
{ // node was deleted because it was unfunded
protected:
	STAmount firstAmount, secondAmount; // Amounts left when declared unfunded
public:
	TMNEUnfunded() : TransactionMetaNodeEntry(TMNDeleteUnfunded) { ; }
	TMNEUnfunded(const STAmount& f, const STAmount& s) :
		TransactionMetaNodeEntry(TMNDeleteUnfunded), firstAmount(f), secondAmount(s) { ; }
	void setBalances(const STAmount& firstBalance, const STAmount& secondBalance);
	virtual void addRaw(Serializer&) const;
	virtual Json::Value getJson(int) const;
	virtual int compare(const TransactionMetaNodeEntry&) const;
	virtual TransactionMetaNodeEntry* duplicate(void) const { return new TMNEUnfunded(*this); }
};

inline TransactionMetaNodeEntry* new_clone(const TransactionMetaNodeEntry& s)	{ return s.clone().release(); }
inline void delete_clone(const TransactionMetaNodeEntry* s)						{ boost::checked_delete(s); }

class TransactionMetaNode
{ // a node that has been affected by a transaction
public:
	typedef boost::shared_ptr<TransactionMetaNode> pointer;

protected:
	uint256 mNode;
	uint256 mPreviousTransaction;
	uint32 mPreviousLedger;
	boost::ptr_vector<TransactionMetaNodeEntry> mEntries;

public:
	TransactionMetaNode(const uint256 &node) : mNode(node) { ; }

	const uint256& getNode() const												{ return mNode; }
	const uint256& getPreviousTransaction() const								{ return mPreviousTransaction; }
	uint32 getPreviousLedger() const											{ return mPreviousLedger; }
	const boost::ptr_vector<TransactionMetaNodeEntry>& peekEntries() const		{ return mEntries; }

	TransactionMetaNodeEntry* findEntry(int nodeType);
	void addNode(TransactionMetaNodeEntry*);

	bool operator<(const TransactionMetaNode& n) const	{ return mNode < n.mNode; }
	bool operator<=(const TransactionMetaNode& n) const	{ return mNode <= n.mNode; }
	bool operator>(const TransactionMetaNode& n) const	{ return mNode > n.mNode; }
	bool operator>=(const TransactionMetaNode& n) const	{ return mNode >= n.mNode; }

	void thread(const uint256& prevTx, uint32 prevLgr);

	TransactionMetaNode(const uint256&node, SerializerIterator&);
	void addRaw(Serializer&);
	Json::Value getJson(int) const;
};


class TransactionMetaSet
{
protected:
	uint256 mTransactionID;
	uint32 mLedger;
	std::map<uint256, TransactionMetaNode> mNodes;

	TransactionMetaNode& modifyNode(const uint256&);

public:
	TransactionMetaSet() : mLedger(0) { ; }
	TransactionMetaSet(const uint256& txID, uint32 ledger) : mTransactionID(txID), mLedger(ledger) { ; }
	TransactionMetaSet(uint32 ledger, const std::vector<unsigned char>&);

	void init(const uint256& transactionID, uint32 ledger);
	void clear() { mNodes.clear(); }
	void swap(TransactionMetaSet&);

	bool isNodeAffected(const uint256&) const;
	const TransactionMetaNode& peekAffectedNode(const uint256&) const;

	Json::Value getJson(int) const;
	void addRaw(Serializer&);

	void threadNode(const uint256& node, const uint256& previousTransaction, uint32 previousLedger);
	bool signedBy(const uint256& node, const STAmount& fee);
	bool deleteUnfunded(const uint256& node, const STAmount& firstBalance, const STAmount& secondBalance);
	bool adjustBalance(const uint256& node, unsigned flags, const STAmount &amount);
	bool adjustBalances(const uint256& node, unsigned flags, const STAmount &firstAmt, const STAmount &secondAmt);
};

#endif

// vim:ts=4
