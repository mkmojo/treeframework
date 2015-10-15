/*
 * DataUtils.hpp
 *
 *  Created on: Jul 3, 2015
 *      Author: samindaw
 */

#ifndef LIB_DATAUTILS_HPP_
#define LIB_DATAUTILS_HPP_

#include <sstream>
#include <cstring>


namespace data_utils{

	size_t copyData(void* mem, const void* p, size_t size){
		std::memcpy(mem, p, size);
		return size;
	}

}

#endif /* LIB_DATAUTILS_HPP_ */
