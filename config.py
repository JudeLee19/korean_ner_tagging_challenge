import os
from general_utils import get_logger


class Config():
    def __init__(self):
        # directory for training outputs
        if not os.path.exists(self.output_path):
            os.makedirs(self.output_path)

        # create instance of logger
        self.logger = get_logger(self.log_path)
        

    # general config
    #output_path = "data/model/"
    output_path = "data/model_1012/"
    #output_path = "results/full_cnn_with_mor_lex_new_data_1008/"
    model_output = output_path + "model_fil_2345_600_100_128_drop_0.8_0924.weights"
    log_path = output_path + "log.txt"

    # word2vec and fast-text 300 dim
    # dim = 300
    
    # glove 100 dim
    dim = 100
    
    # dim_char = 100
    
    
    # glove_filename = "data/korean_data/word2vec/korean_word2vec_{}_dim".format(dim)
    glove_filename = "data/korean_data/glove/korean_glove_100_dim"
    # glove_filename = 'data/korean_data/fast_text/wiki.ko.vec'
    
    
    # trimmed embeddings (created from glove_filename with build_dat
    # trimmed_filename = "data/korean_data/word2vec/korean_word2vec_{}_dim_trimmed.npz".format(dim)
    trimmed_filename = "data/korean_data/glove/korean_glove_100_dim_trimmed.npz"
    # trimmed_filename = "data/korean_data/fast_text/korean_fast_text_100_dim_trimmed.npz"

    # dataset
    
    # no shuffle
    #train_filename = "data/challenge_data/train_corpus_with_mor_new_09_24"
    #dev_filename = "data/challenge_data/dev_corpus_with_mor_new_09_24"
    #test_filename = "data/challenge_data/test_corpus_with_mor_new_09_24"
    
    # # shuffle
    # train_filename = "data/challenge_data/shuffle_train_corpus_with_mor_new_09_23"
    # dev_filename = "data/challenge_data/shuffle_dev_corpus_with_mor_new_09_23"
    # test_filename = "data/challenge_data/shuffle_test_corpus_with_mor_new_09_23"
    
    # full training
    train_filename = "data/challenge_data/full_train_corpus_with_mor_new_09_24"
    
    train_filename = 'data/challenge_data/mor_train_data_1008/full_train_corpus_with_mor_new_10_12'
    dev_filename = "data/challenge_data/dev_corpus_with_mor_new_09_24"
    test_filename = "data/challenge_data/test_corpus_with_mor_new_09_24"
    
    max_iter = None # if not None, max number of examples

    # vocab (created from dataset with build_data.py)
    # words_filename = "full_new_data_glove_100_0924/words.txt"
    # mor_tags_filename = 'full_new_data_glove_100_0924/mor_tags.txt'
    # tags_filename = "full_new_data_glove_100_0924/tags.txt"
    # chars_filename = "full_new_data_glove_100_0924/chars.txt"
    # lex_tags_filename = 'full_new_data_glove_100_0924/lex_tags.txt'
    
    # real demo
    words_filename = "data_vocab/words.txt"
    mor_tags_filename = 'data_vocab/mor_tags.txt'
    tags_filename = "data_vocab/tags.txt"
    chars_filename = "data_vocab/chars.txt"
    lex_tags_filename = 'data_vocab/lex_tags.txt'

    words_filename = "data_vocab_1012/words.txt"
    mor_tags_filename = 'data_vocab_1012/mor_tags.txt'
    tags_filename = "data_vocab_1012/tags.txt"
    chars_filename = "data_vocab_1012/chars.txt"
    lex_tags_filename = 'data_vocab_1012/lex_tags.txt'
    
    # training
    # train_embeddings = False
    train_embeddings = True
    nepochs = 17
    dropout = 0.8
    cnn_dropout = 0.8
    batch_size = 20
    lr_method = "adam"
    lr = 0.01
    lr_decay = 0.9
    clip = -1 # if negative, no clipping
    nepoch_no_imprv = 3
    reload = False
    
    # model hyperparameters
    hidden_size = 600

    dim_char = 100
    char_hidden_size = 100
    
    # NOTE: if both chars and crf, only 1.6x slower on GPU
    crf = True # if crf, training is 1.7x slower on CPU
    chars = True # if char embedding, training is 3.5x slower on CPU

