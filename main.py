from data_utils import get_trimmed_glove_vectors, load_vocab, \
    get_processing_word, Data
from config import Config
from cnn_lstm_crf import CnnLstmCrfModel


def main(config):
    # load vocabs
    vocab_words = load_vocab(config.words_filename)
    vocab_mor_tags = load_vocab(config.mor_tags_filename)
    vocab_tags = load_vocab(config.tags_filename)
    vocab_chars = load_vocab(config.chars_filename)
    vocab_lex_tags = load_vocab(config.lex_tags_filename)
    
    # get processing functions
    processing_word = get_processing_word(vocab_words, vocab_chars,
                                          lowercase=True, chars=config.chars)
    processing_mor_tag = get_processing_word(vocab_mor_tags, lowercase=False)
    processing_tag = get_processing_word(vocab_tags,
                                         lowercase=False)
    processing_lex_tag = get_processing_word(vocab_lex_tags, lowercase=False)
    
    # get pre trained embeddings
    embeddings = get_trimmed_glove_vectors(config.trimmed_filename)
    
    # create dataset
    dev = Data(config.dev_filename, processing_word, processing_mor_tag,
               processing_lex_tag, processing_tag, config.max_iter)
    test = Data(config.test_filename, processing_word, processing_mor_tag,
                processing_lex_tag, processing_tag, config.max_iter)
    train = Data(config.train_filename, processing_word, processing_mor_tag,
                 processing_lex_tag, processing_tag, config.max_iter)
    
    cnn_model = CnnLstmCrfModel(config, embeddings, ntags=len(vocab_tags), nchars=len(vocab_chars))
    cnn_model.build()
    cnn_model.train(train, dev, vocab_tags)
    cnn_model.evaluate(test, vocab_tags)


if __name__ == "__main__":
    # create instance of config
    config = Config()
    
    # load, train, evaluate and interact with model
    main(config)