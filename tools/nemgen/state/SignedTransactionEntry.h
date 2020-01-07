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
				, m_hexPayload("")
				, m_rawPayload{}
		{
		}

		/// Creates an entry around \a transaction and \a signer.
		SignedTransactionEntry(const std::string& payload, const Key& signer)
				: m_pSigner(&signer)
				, m_hexPayload(payload)
		{
			// - parse transaction payload to binary
			m_rawPayload = std::vector<uint8_t>(m_hexPayload.size() / 2);
			auto parseResult = utils::TryParseHexStringIntoContainer(
				m_hexPayload.c_str(),
				m_hexPayload.size(),
				m_rawPayload
			);

			if (false == parseResult)
				CATAPULT_THROW_INVALID_ARGUMENT_1("transaction payload must be hexadecimal", m_hexPayload);
		}

	public:
		/// Gets the signer.
		const Key& signer() const {
			return *m_pSigner;
		}

		/// Gets the hexadecimal payload.
		std::string hexPayload() const {
			return m_hexPayload;
		}

		/// Gets the raw payload.
		const std::vector<uint8_t>& rawPayload() const {
			return m_rawPayload;
		}

	private:
		const Key* m_pSigner;
		std::string m_hexPayload;
		std::vector<uint8_t> m_rawPayload;
	};
}}
