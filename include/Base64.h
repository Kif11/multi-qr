#ifndef BASE64CODER_H    // To make sure you don't declare the function more than once by including the header multiple times.
#define BASE64CODER_H

std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len);
std::string base64_decode(std::string const& encoded_string);

#endif