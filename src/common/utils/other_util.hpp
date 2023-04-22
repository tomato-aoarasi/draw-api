/*
 * @File	  : other_util.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/04/22 15:33
 * @Introduce : 其他工具
*/

class OtherUtil final {
public:
    std::string base64_encode(const unsigned char* input, int length) {
        BIO* bmem, * b64;
        BUF_MEM* bptr;

        b64 = BIO_new(BIO_f_base64());
        bmem = BIO_new(BIO_s_mem());
        b64 = BIO_push(b64, bmem);
        BIO_write(b64, input, length);
        BIO_flush(b64);
        BIO_get_mem_ptr(b64, &bptr);

        std::string result(bptr->data, bptr->length);

        BIO_free_all(b64);

        result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
        result.erase(std::remove(result.begin(), result.end(), '\r'), result.end());

        delete bmem;
        delete b64;
        delete bptr;
        return result;
    }
private:
};
