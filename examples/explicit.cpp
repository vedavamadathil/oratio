#include "nabu.hpp"

using namespace nabu;

StringFeeder sf = R"(123 456)";

struct myrule {};

template <>
struct rules::rule <myrule> : public rules::rule <
                rules::skipper <int>
        > {};

set_id(myrule, 1);

int main()
{
        // ret rptr = rules::kstar <myrule> ::value(&sf);
        // std::cout << getrv(rptr).json() << std::endl;

        parser::lexicon lptr;
        lptr = parser::lexer <rules::skipper <int>> (&sf);
        std::cout << "Code [1]: " << lptr->id << ": " << get <int> (lptr) << std::endl;
        lptr = parser::lexer <rules::skipper <int>> (&sf);
        std::cout << "Code [2]: " << lptr->id << ": " << get <int> (lptr) << std::endl;
}