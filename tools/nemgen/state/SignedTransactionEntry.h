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
				, m_hexPayloads{}
				, m_rawPayloads{}
		{
		}

		/// Creates an entry around \a transaction and \a signer.
		SignedTransactionEntry(const std::string& payload, const Key& signer)
				: m_pSigner(&signer)
				, m_hexPayloads{payload}
		{
			m_rawPayloads.push_back(TryParsePayload(payload));
		}

		size_t AddTransaction(const std::string& payload) const {
			//XXX verify/validate size

			m_rawPayloads.push_back(TryParsePayload(payload));
			m_hexPayloads.push_back(payload);

			return size();
		}

	protected:

		std::vector<uint8_t> TryParsePayload(const std::string& payload) {
			// - parse transaction payload to binary
			auto rawPayload = std::vector<uint8_t>(payload.size() / 2);
			auto parseResult = utils::TryParseHexStringIntoContainer(
				payload.c_str(),
				payload.size(),
				rawPayload
			);

			if (false == parseResult)
				CATAPULT_THROW_INVALID_ARGUMENT_1("transaction payload must be hexadecimal", payload);

			return rawPayload;
		} 

	public:
		/// Gets the signer.
		const Key& signer() const {
			return *m_pSigner;
		}

		/// Gets the hexadecimal payload.
		std::vector<std::string> payloads() const {
			return m_hexPayloads;
		}

		/// Gets the raw payload.
		const std::vector<std::vector<uint8_t>>& rawPayloads() const {
			return m_rawPayloads;
		}

		/// Gets the number of signed payloads.
		size_t size() const {
			return m_rawPayloads.size();
		}

	private:
		const Key* m_pSigner;
		std::vector<std::string> m_hexPayloads;
		std::vector<std::vector<uint8_t>> m_rawPayloads;
	};
}}
