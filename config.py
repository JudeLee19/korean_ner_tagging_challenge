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
    # output_path = "data/model_1012/"
    # output_path = 'data/model_1221/'
    
    # 전통문화
    output_path = 'data/model_0611/'
    model_output = output_path + "model_.weights"
    log_path = output_path + "log.txt"

    # word2vec and fast-text 300 dim
    # dim = 300
    
    # glove 100 dim
    dim = 100
    
    # dim_char = 100
    
    
    # glove_filename = "data/korean_data/word2vec/korean_word2vec_{}_dim".format(dim)
    glove_filename = "data/korean_data/glove/korean_glove_100_dim"
    
    
    trimmed_filename = "data/korean_data/glove/korean_glove_100_dim_trimmed.npz"

    # full training
    train_filename = "data/data_file/train_filename"
    dev_filename = "data/data_file/dev_filename"
    test_filename = "data/data_file/test_filename"
    
    max_iter = None # if not None, max number of examples
    
    
    # 전통문화
    words_filename = "data_vocab_0611/words.txt"
    mor_tags_filename = 'data_vocab_0611/mor_tags.txt'
    tags_filename = "data_vocab_0611/tags.txt"
    chars_filename = "data_vocab_0611/chars.txt"
    lex_tags_filename = 'data_vocab_0611/lex_tags.txt'

    # real demo
    # words_filename = "data_vocab_1221/words.txt"
    # mor_tags_filename = 'data_vocab_1221/mor_tags.txt'
    # tags_filename = "data_vocab_1221/tags.txt"
    # chars_filename = "data_vocab_1221/chars.txt"
    # lex_tags_filename = 'data_vocab_1221/lex_tags.txt'
    
    
    
    # training
    # train_embeddings = False
    train_embeddings = True
    nepochs = 20
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

