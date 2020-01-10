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

	/// 
	class SignedTransactionPayload {
	public:
		/// Creates an uninitialized entry.
		SignedTransactionPayload()
				: m_hexPayload("")
				, m_rawPayload{}
		{
		}

		/// Creates a payyload around \a payload and \a signer.
		SignedTransactionPayload(const std::string& payload)
				: m_hexPayload(payload)
		{
			m_rawPayload = std::vector<uint8_t>(payload.size() / 2);
			TryParsePayload(payload, m_rawPayload);
		}

	protected:

		size_t TryParsePayload(const std::string& payload, std::vector<uint8_t>& destination) {
			// - parse transaction payload to binary
			auto parseResult = utils::TryParseHexStringIntoContainer(
				payload.c_str(),
				payload.size(),
				destination
			);

			if (false == parseResult)
				CATAPULT_THROW_INVALID_ARGUMENT_1("transaction payload must be hexadecimal", payload);

			return payload.size();
		} 

	public:
		/// Gets the hexadecimal payload.
		std::string toHex() const {
			return m_hexPayload;
		}

		/// Gets the raw payload.
		const std::vector<uint8_t>& toBinary() const {
			return m_rawPayload;
		}

		/// Gets the size of the signed transaction.
		size_t size() const {
			return m_rawPayload.size();
		}

	private:
		std::string m_hexPayload;
		std::vector<uint8_t> m_rawPayload;
	};
}}
