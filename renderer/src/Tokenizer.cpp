#include "Tokenizer.hpp"

#include "EPubTokenizer.hpp"
#include "NullTokenizer.hpp"
#include "RendererStream.hpp"
#include "TxtTokenizer.hpp"

auto Tokenizer::factory(const std::shared_ptr<RendererStream> &stream, RendererDisplay &display,
                        RendererDisplayRect &rect, Tokens &tokens, Pager &pager, FontHandle font)
    -> std::shared_ptr<Tokenizer> {
    std::shared_ptr<Tokenizer> tokenizer;

    if (stream->isFor(DocType::TXT)) {
        tokenizer = std::make_shared<TxtTokenizer>(display, rect, tokens, pager, font);
    } else if (stream->isFor(DocType::EPUB)) {
        tokenizer = std::make_shared<EPubTokenizer>(display, rect, tokens, pager, font);
    } else {
        LOGE(REND_TAG, "Stream is of wrong type(must be TXT or EPUB) ! : %" PRIu8,
             static_cast<uint8_t>(stream->getDocType()));
        REND_EXIT_SIM;
        tokenizer = std::make_shared<NullTokenizer>(display, rect, tokens, pager, font);
    }
    tokenizer->setStream(stream);
    pager.setStream(stream);

    return tokenizer;
}
