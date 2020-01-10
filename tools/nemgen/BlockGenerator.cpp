/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "BlockGenerator.h"
#include "NemesisExecutionHasher.h"
#include "TransactionRegistryFactory.h"
#include "catapult/builders/MosaicAliasBuilder.h"
#include "catapult/builders/MosaicDefinitionBuilder.h"
#include "catapult/builders/MosaicSupplyChangeBuilder.h"
#include "catapult/builders/NamespaceRegistrationBuilder.h"
#include "catapult/builders/TransferBuilder.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/crypto/KeyUtils.h"
#include "catapult/crypto/Signer.h"
#include "catapult/extensions/BlockExtensions.h"
#include "catapult/extensions/ConversionExtensions.h"
#include "catapult/extensions/IdGenerator.h"
#include "catapult/extensions/TransactionExtensions.h"
#include "catapult/model/Address.h"
#include "catapult/model/Block.h"
#include "catapult/model/EntityHasher.h"
#include "catapult/model/Transaction.h"
#include "catapult/utils/HexParser.h"
#include "catapult/utils/HexFormatter.h"

namespace catapult { namespace tools { namespace nemgen {

	namespace {
		size_t CalculateSizeOfPaddedNemesisBlock(model::Block& block, const model::Transactions& transactions) {
			// last nemesis transaction gets padded (if necessary)
			const auto last = transactions.size() - 1;
			return block.Size + utils::GetPaddingSize(transactions[last]->Size, 8);
		}

		size_t CalculateSizeWithAddedSignedTransactions(
			model::Block& block,
			const model::Transactions& transactions,
			SignerToSignedTransactionMap signedTransactions) {
			auto newSize = CalculateSizeOfPaddedNemesisBlock(block, transactions);
			auto lastPadding = 0u;

			// each signed transaction get padded
			for (const auto& transactionPayloadPair : signedTransactions) {
				auto transactionEntry = transactionPayloadPair.second;
				auto transactionBinaries = transactionEntry.rawPayloads();
				for (const auto& transactionBinary : transactionBinaries) {
					lastPadding = utils::GetPaddingSize(transactionBinary.size(), 8);
					newSize += transactionBinary.size() + lastPadding;
				}
			}

			// last signed transaction not padded
			newSize -= lastPadding;
			return newSize;
		}

		std::unique_ptr<model::Block> CopyBlockReallocate(model::Block& block, size_t oldSize, size_t newSize) {
			auto pNewBlock = utils::MakeUniqueWithSize<model::Block>(newSize);
			std::memset(static_cast<void*>(pNewBlock.get()), 0, newSize);
			std::memcpy(static_cast<void*>(pNewBlock.get()), &block, oldSize);
			pNewBlock->Size = newSize;
			return pNewBlock;
		}

		size_t AddLastTransactionPadding(uint8_t* pDestination, const model::Transactions& transactions) {
			// 1) add padding for last nemesis transaction
			const auto last = transactions.size() - 1;
			auto lastTransaction = transactions[last];
			auto paddingSize = utils::GetPaddingSize(lastTransaction->Size, 8);

			CATAPULT_LOG(debug) << "Padding last nemesis transaction";
			CATAPULT_LOG(debug) << "- Last Transaction Size: " << lastTransaction->Size;
			CATAPULT_LOG(debug) << "- Padding Needed Size: " << paddingSize;

			std::memset(static_cast<void*>(pDestination), 0, paddingSize);
			pDestination += paddingSize;
			return paddingSize;
		}

		void AddSignedTransactions(uint8_t* pDestination, SignerToSignedTransactionMap signedTransactions) {
			CATAPULT_LOG(debug);
			CATAPULT_LOG(debug) << "Appending signed transactions";
			CATAPULT_LOG(debug) << "- Count Signed Transaction Payloads: " << signedTransactions.size();

			// 2) each signed transaction is appended and padded
			auto i = 0u;
			auto total = 0u;
			for (const auto& transactionPayloadPair : signedTransactions) {
				auto transactionEntry = transactionPayloadPair.second;
				auto transactionBinaries = transactionEntry.rawPayloads();

				auto j = 0u;
				for (const auto& transactionBinary : transactionBinaries) {

					CATAPULT_LOG(debug) << "Appending signed transaction payload (" << total << ")";
					CATAPULT_LOG(debug) << "- Transaction Payload: " << transactionEntry.payloads().at(j);
					CATAPULT_LOG(debug) << "- Transaction Size: " << transactionBinary.size();

					// copy transaction data into block
					std::memcpy(pDestination, transactionBinary.data(), transactionBinary.size());
					pDestination += transactionBinary.size();

					// pad transaction data only if more available
					bool hasRemaining = (i < signedTransactions.size() - 1) || (
						i == signedTransactions.size() - 1 && j < transactionBinaries.size() - 1
					);

					if (hasRemaining) {
						auto paddingSize = utils::GetPaddingSize(transactionBinary.size(), 8);

						CATAPULT_LOG(debug) << "Padding signed transaction payload (" << i << ")";
						CATAPULT_LOG(debug) << "- Padding Needed Size: " << paddingSize;

						std::memset(static_cast<void*>(pDestination), 0, paddingSize);
						pDestination += paddingSize;
					}
					++j;
					++total;
				}
				++i;
			}
		}

		std::unique_ptr<model::Block> AppendSignedTransactions(
			model::Block& block,
			const model::Transactions& transactions,
			SignerToSignedTransactionMap signedTransactions) {
			// - calculate old size with added padding and new size with signed transactions
			uint32_t oldSize = CalculateSizeOfPaddedNemesisBlock(block, transactions);
			uint32_t newSize = CalculateSizeWithAddedSignedTransactions(
				block,
				transactions,
				signedTransactions
			);

			CATAPULT_LOG(debug) << "Resizing block";
			CATAPULT_LOG(debug) << "- Old Size: " << block.Size;
			CATAPULT_LOG(debug) << "- New Size: " << newSize;

			// - re-create block with extended size
			auto pNewBlock = CopyBlockReallocate(block, oldSize, newSize);

			CATAPULT_LOG(debug) << "Copied old block";
			CATAPULT_LOG(debug) << "Old block size: " << block.Size;
			CATAPULT_LOG(debug) << "New block size: " << pNewBlock->Size;

			// - add signed transactions data *after* nemesis transactions
			auto cursorPosition = oldSize - sizeof(model::BlockHeader);
			auto* pDestination = reinterpret_cast<uint8_t*>(pNewBlock->TransactionsPtr()) + cursorPosition;
			AddLastTransactionPadding(pDestination, transactions);
			AddSignedTransactions(pDestination, signedTransactions);

			CATAPULT_LOG(debug) << "Signed transactions added";
			return pNewBlock;
		}
	}

	namespace {
		std::string FixName(const std::string& mosaicName) {
			auto name = mosaicName;
			for (auto& ch : name) {
				if (':' == ch)
					ch = '.';
			}

			return name;
		}

		std::string GetChildName(const std::string& namespaceName) {
			return namespaceName.substr(namespaceName.rfind('.') + 1);
		}

		class NemesisTransactions {
		public:
			NemesisTransactions(
					model::NetworkIdentifier networkIdentifier,
					const GenerationHash& generationHash,
					const crypto::KeyPair& signer)
					: m_networkIdentifier(networkIdentifier)
					, m_generationHash(generationHash)
					, m_signer(signer)
			{}

		public:
			void addNamespaceRegistration(const std::string& namespaceName, BlockDuration duration) {
				builders::NamespaceRegistrationBuilder builder(m_networkIdentifier, m_signer.publicKey());
				builder.setName({ reinterpret_cast<const uint8_t*>(namespaceName.data()), namespaceName.size() });
				builder.setDuration(duration);
				signAndAdd(builder.build());
			}

			void addNamespaceRegistration(const std::string& namespaceName, NamespaceId parentId) {
				builders::NamespaceRegistrationBuilder builder(m_networkIdentifier, m_signer.publicKey());
				builder.setName({ reinterpret_cast<const uint8_t*>(namespaceName.data()), namespaceName.size() });
				builder.setParentId(parentId);
				signAndAdd(builder.build());
			}

			MosaicId addMosaicDefinition(MosaicNonce nonce, const model::MosaicProperties& properties) {
				builders::MosaicDefinitionBuilder builder(m_networkIdentifier, m_signer.publicKey());
				builder.setNonce(nonce);
				builder.setFlags(properties.flags());
				builder.setDivisibility(properties.divisibility());
				builder.setDuration(properties.duration());

				auto pTransaction = builder.build();
				auto id = pTransaction->Id;
				signAndAdd(std::move(pTransaction));
				return id;
			}

			UnresolvedMosaicId addMosaicAlias(const std::string& mosaicName, MosaicId mosaicId) {
				auto namespaceName = FixName(mosaicName);
				auto namespacePath = extensions::GenerateNamespacePath(namespaceName);
				auto namespaceId = namespacePath[namespacePath.size() - 1];
				builders::MosaicAliasBuilder builder(m_networkIdentifier, m_signer.publicKey());
				builder.setNamespaceId(namespaceId);
				builder.setMosaicId(mosaicId);
				builder.setAliasAction(model::AliasAction::Link);
				auto pTransaction = builder.build();
				signAndAdd(std::move(pTransaction));

				CATAPULT_LOG(debug)
						<< "added alias from ns " << utils::HexFormat(namespaceId) << " (" << namespaceName
						<< ") -> mosaic " << utils::HexFormat(mosaicId);
				return UnresolvedMosaicId(namespaceId.unwrap());
			}

			void addMosaicSupplyChange(UnresolvedMosaicId mosaicId, Amount delta) {
				builders::MosaicSupplyChangeBuilder builder(m_networkIdentifier, m_signer.publicKey());
				builder.setMosaicId(mosaicId);
				builder.setAction(model::MosaicSupplyChangeAction::Increase);
				builder.setDelta(delta);
				auto pTransaction = builder.build();
				signAndAdd(std::move(pTransaction));
			}

			void addTransfer(
					const std::map<std::string, UnresolvedMosaicId>& mosaicNameToMosaicIdMap,
					const Address& recipientAddress,
					const std::vector<MosaicSeed>& seeds) {
				auto recipientUnresolvedAddress = extensions::CopyToUnresolvedAddress(recipientAddress);
				builders::TransferBuilder builder(m_networkIdentifier, m_signer.publicKey());
				builder.setRecipientAddress(recipientUnresolvedAddress);
				for (const auto& seed : seeds) {
					auto mosaicId = mosaicNameToMosaicIdMap.at(seed.Name);
					builder.addMosaic({ mosaicId, seed.Amount });
				}

				signAndAdd(builder.build());
			}

		public:
			const model::Transactions& transactions() const {
				return m_transactions;
			}

		private:
			void signAndAdd(std::unique_ptr<model::Transaction>&& pTransaction) {
				pTransaction->Deadline = Timestamp(1);
				extensions::TransactionExtensions(m_generationHash).sign(m_signer, *pTransaction);
				m_transactions.push_back(std::move(pTransaction));
			}

		private:
			model::NetworkIdentifier m_networkIdentifier;
			const GenerationHash& m_generationHash;
			const crypto::KeyPair& m_signer;
			model::Transactions m_transactions;
		};
	}

	std::unique_ptr<model::Block> CreateNemesisBlock(const NemesisConfiguration& config) {
		auto signer = crypto::KeyPair::FromString(config.NemesisSignerPrivateKey);
		NemesisTransactions transactions(config.NetworkIdentifier, config.NemesisGenerationHash, signer);

		// - namespace creation
		for (const auto& rootPair : config.RootNamespaces) {
			// - root
			const auto& root = rootPair.second;
			const auto& rootName = config.NamespaceNames.at(root.id());
			auto duration = std::numeric_limits<BlockDuration::ValueType>::max() == root.lifetime().End.unwrap()
					? Eternal_Artifact_Duration
					: BlockDuration((root.lifetime().End - root.lifetime().Start).unwrap());
			transactions.addNamespaceRegistration(rootName, duration);

			// - children
			std::map<size_t, std::vector<state::Namespace::Path>> paths;
			for (const auto& childPair : root.children()) {
				const auto& path = childPair.second.Path;
				paths[path.size()].push_back(path);
			}

			for (const auto& pair : paths) {
				for (const auto& path : pair.second) {
					const auto& child = state::Namespace(path);
					auto subName = GetChildName(config.NamespaceNames.at(child.id()));
					transactions.addNamespaceRegistration(subName, child.parentId());
				}
			}
		}

		// - mosaic creation
		MosaicNonce nonce;
		std::map<std::string, UnresolvedMosaicId> nameToMosaicIdMap;
		for (const auto& mosaicPair : config.MosaicEntries) {
			const auto& mosaicEntry = mosaicPair.second;

			// - definition
			auto mosaicId = transactions.addMosaicDefinition(nonce, mosaicEntry.definition().properties());
			CATAPULT_LOG(debug) << "mapping " << mosaicPair.first << " to " << utils::HexFormat(mosaicId) << " (nonce " << nonce << ")";
			nonce = nonce + MosaicNonce(1);

			// - alias
			auto unresolvedMosaicId = transactions.addMosaicAlias(mosaicPair.first, mosaicId);
			nameToMosaicIdMap.emplace(mosaicPair.first, unresolvedMosaicId);

			// - supply
			transactions.addMosaicSupplyChange(unresolvedMosaicId, mosaicEntry.supply());
		}

		// - mosaic distribution
		for (const auto& addressMosaicSeedsPair : config.NemesisAddressToMosaicSeeds) {
			auto recipient = model::StringToAddress(addressMosaicSeedsPair.first);
			transactions.addTransfer(nameToMosaicIdMap, recipient, addressMosaicSeedsPair.second);
		}

		model::PreviousBlockContext context;
		auto pBlock = model::CreateBlock(context, config.NetworkIdentifier, signer.publicKey(), transactions.transactions());
		pBlock->Type = model::Entity_Type_Nemesis_Block;

		if (1 <= config.SignedTransactionEntries.size()) {
			auto pBlockExtended = AppendSignedTransactions(*pBlock, transactions.transactions(), config.SignedTransactionEntries);

			CATAPULT_LOG(debug) << "Signing extended nemesis block with size: " << pBlockExtended->Size;

			extensions::BlockExtensions(config.NemesisGenerationHash).signFullBlock(signer, *pBlockExtended);
			return pBlockExtended;
		}
		
		extensions::BlockExtensions(config.NemesisGenerationHash).signFullBlock(signer, *pBlock);
		return pBlock;
	}

	Hash256 UpdateNemesisBlock(
			const NemesisConfiguration& config,
			model::Block& block,
			NemesisExecutionHashesDescriptor& executionHashesDescriptor) {
		block.ReceiptsHash = executionHashesDescriptor.ReceiptsHash;
		block.StateHash = executionHashesDescriptor.StateHash;

		auto signer = crypto::KeyPair::FromString(config.NemesisSignerPrivateKey);
		extensions::BlockExtensions(config.NemesisGenerationHash).signFullBlock(signer, block);
		return model::CalculateHash(block);
	}

	model::BlockElement CreateNemesisBlockElement(const NemesisConfiguration& config, const model::Block& block) {
		auto registry = CreateTransactionRegistry();
		auto generationHash = config.NemesisGenerationHash;
		return extensions::BlockExtensions(generationHash, registry).convertBlockToBlockElement(block, generationHash);
	}
}}}
