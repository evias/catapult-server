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

#pragma once
#include "SignedTransactionPayload.h"
#include "catapult/model/Transaction.h"
#include "catapult/exceptions.h"
#include "catapult/utils/HexParser.h"
#include <vector>
#include <string>

namespace catapult { namespace state {

	/// Pair composed of a signer public key and a signed transaction payload
	class SignedTransactionEntry {
	public:
		/// Creates an uninitialized entry.
		SignedTransactionEntry()
				: m_pSigner(nullptr)
				, m_payloads{}
		{
		}

		/// Creates an entry around \a payload and \a signer.
		SignedTransactionEntry(const std::string& payload, const Key& signer)
				: m_pSigner(&signer)
				, m_payloads{}
		{
			const auto& pPayload = std::make_unique<SignedTransactionPayload>(payload);
			m_payloads.push_back(std::move(pPayload));
		}

		size_t AddTransaction(const std::string& payload) const {
			const auto& pPayload = std::make_unique<SignedTransactionPayload>(payload);
			m_payloads.push_back(std::move(pPayload));
			return size();
		}

	public:
		/// Gets the signer.
		const Key& signer() const {
			return *m_pSigner;
		}

		/// Gets the signed transaction payloads.
		const std::vector<SignedTransactionPayload>& payloads() const {
			return m_payloads;
		}

		/// Gets the number of signed payloads.
		size_t size() const {
			return m_payloads.size();
		}

	private:
		const Key* m_pSigner;
		std::vector<std::unique_ptr<SignedTransactionPayload>> m_payloads;
	};
}}
