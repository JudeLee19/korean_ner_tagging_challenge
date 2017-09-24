#-*-coding: utf-8
import numpy as np
import os
import tensorflow as tf
from data_utils import minibatches, pad_sequences, get_chunks
from general_utils import Progbar
import subprocess
import joblib
import re

UNK = "$UNK$"
NUM = "$NUM$"
NONE = "O"


class CnnLstmCrfModel(object):
    def __init__(self, config, embeddings, ntags, nchars=None):
        """
        Args:
            config: class with hyper parameters
            embeddings: np array with embeddings
            nchars: (int) size of chars vocabulary
        """
        self.config = config
        self.embeddings = embeddings
        self.nchars = nchars
        self.ntags = ntags
        self.logger = config.logger  # now instantiated in config

        filter_sizes_str = '2,3,4,5'
        filter_sizes = list(map(int, filter_sizes_str.split(",")))
        self.filter_sizes = filter_sizes
        self.num_filters = 128
        self.l2_reg_lambda = 0.0
        self.cnn_word_lengths = 15

        self.lex_dict = joblib.load('./data/gazette/lex_dict')
        
    def add_placeholders(self):
        """
        Adds placeholders to self
        """
        
        # shape = (batch size, max length of sentence in batch)
        self.word_ids = tf.placeholder(tf.int32, shape=[None, None],
                                       name="word_ids")
        
        # shape = (batch size)
        self.sequence_lengths = tf.placeholder(tf.int32, shape=[None],
                                               name="sequence_lengths")
        
        # shape = (batch size, max length of sentence, max length of word)
        self.char_ids = tf.placeholder(tf.int32, shape=[None, None, None],
                                       name="char_ids")
        
        # shape = (batch_size, max_length of sentence)
        self.word_lengths = tf.placeholder(tf.int32, shape=[None, None],
                                           name="word_lengths")
        
        # shape = (batch size, max length of sentence in batch)
        self.mor_tags = tf.placeholder(tf.int32, shape=[None, None],
                                     name="mor_tags")

        # shape = (batch size, max length of sentence in batch, lexicon_tag_size)
        self.lex_tags = tf.placeholder(tf.float32, shape=[None, None, 6],
                                       name="lexicon_tags")
        
        # shape = (batch size, max length of sentence in batch)
        self.labels = tf.placeholder(tf.int32, shape=[None, None],
                                     name="labels")
        
        # hyper parameters
        self.dropout = tf.placeholder(dtype=tf.float32, shape=[],
                                      name="dropout")
        self.lr = tf.placeholder(dtype=tf.float32, shape=[],
                                 name="lr")
    
    def get_feed_dict(self, words, mor_tags=None, lex_tags=None, labels=None, lr=None, dropout=None):
        """
        Given some data, pad it and build a feed dictionary
        Args:
            words: list of sentences. A sentence is a list of ids of a list of words.
                A word is a list of ids
            labels: list of ids
            lr: (float) learning rate
            dropout: (float) keep prob
        Returns:
            dict {placeholder: value}
        """
        # perform padding of the given data
        if self.config.chars:
            char_ids, word_ids = zip(*words)
            word_ids, sequence_lengths = pad_sequences(word_ids, 0)
            char_ids, word_lengths = pad_sequences(char_ids, pad_tok=0, nlevels=2)
        else:
            word_ids, sequence_lengths = pad_sequences(words, 0)
        
        # build feed dictionary
        feed = {
            self.word_ids: word_ids,
            self.sequence_lengths: sequence_lengths
        }
        
        if self.config.chars:
            feed[self.char_ids] = char_ids
            feed[self.word_lengths] = word_lengths
            self.cnn_word_lengths = word_lengths
        
        if lex_tags is not None:
            lex_tags, _ = pad_sequences(lex_tags, 0)
            # add two hot code here
            batch_arr = []
            for b_i, sentence in enumerate(lex_tags):
                sentence_arr = []
                for w_i, each_word_lex in enumerate(sentence):
        
                    word_lex_hot = list([0.0, 0.0, 0.0, 0.0, 0.0, 0.0])
        
                    if isinstance(each_word_lex, str) and ',' in each_word_lex:
                        for word in each_word_lex.split(','):
                            word_idx = int(word)
                            word_lex_hot[word_idx] = 1.0
                    else:
                        word_lex_hot[each_word_lex] = 1.0
        
                    sentence_arr.append(word_lex_hot)
                batch_arr.append(sentence_arr)
            feed[self.lex_tags] = batch_arr
        
        if mor_tags is not None:
            mor_tags, _ = pad_sequences(mor_tags, 0)
            feed[self.mor_tags] = mor_tags
        
        if labels is not None:
            labels, _ = pad_sequences(labels, 0)
            feed[self.labels] = labels
        
        if lr is not None:
            feed[self.lr] = lr
        
        if dropout is not None:
            feed[self.dropout] = dropout
        
        return feed, sequence_lengths
    
    def add_word_embeddings_op(self):
        """
        Adds word embeddings to self
        """
        with tf.variable_scope("words"):
            W_word_embeddings = tf.Variable(self.embeddings, name="_word_embeddings", dtype=tf.float32,
                                           trainable=self.config.train_embeddings)
            word_embeddings = tf.nn.embedding_lookup(W_word_embeddings, self.word_ids,
                                                     name="word_embeddings")
        with tf.variable_scope("mor_tags"):
            # shape = (batch size, max length of sentence, mor_tag_size)
            mor_tag_embeddings = tf.one_hot(self.mor_tags, depth=42)
            
        with tf.variable_scope("lex_tags"):
            # shape = (batch size, max length of sentence, lexicon_tag_size)
            lex_tag_embeddings = self.lex_tags
            
        with tf.variable_scope("chars"):
            if self.config.chars:
                # get embeddings matrix
                W_char_embeddings = tf.get_variable(name="_char_embeddings", dtype=tf.float32,
                                                   shape=[self.nchars, self.config.dim_char])
                char_embeddings = tf.nn.embedding_lookup(W_char_embeddings, self.char_ids,
                                                         name="char_embeddings")

                # shape = (batch size, max length of sentence, max length of word, char dimension)
                s = tf.shape(char_embeddings)
                
                # shape = (batch size x max length of sentence , max length of word, char dimension)
                char_embeddings = tf.reshape(char_embeddings, shape=[-1, s[-2], self.config.dim_char])
                
                # add channel dimension for cnn input
                char_embeddings = tf.expand_dims(char_embeddings, -1)
                
                # add char embedding dropout before cnn input
                char_embeddings = tf.nn.dropout(char_embeddings, self.dropout)
                
                # convolution expects shape of (batch, width, height, channel=1)
                # input shape : (batch size x max length of sentence , max length of word, char dimension, channel=1)
                # output shape : (batch size x max length of sentence, h_pool_flat)
                
                pooled_outputs = []
                for i, filter_size in enumerate(self.filter_sizes):
                    with tf.name_scope("conv-maxpool-%s" % filter_size):
                        # Convolution Layer
                        filter_shape = [filter_size, self.config.dim_char, 1, self.num_filters]
                        W = tf.Variable(tf.truncated_normal(filter_shape, stddev=0.1), name="W")
                        b = tf.Variable(tf.constant(0.1, shape=[self.num_filters]), name="b")
        
                        conv = tf.nn.conv2d(
                            char_embeddings,
                            W,
                            strides=[1, 1, 1, 1],
                            padding="VALID",
                            name="conv")
        
                        # Apply nonlinearity
                        h = tf.nn.relu(tf.nn.bias_add(conv, b), name="relu")
                        # Maxpooling over the outputs
                        pooled = tf.nn.max_pool(
                            h,
                            ksize=[1, self.cnn_word_lengths - filter_size + 1, 1, 1],
                            strides=[1, 1, 1, 1],
                            padding='VALID',
                            name="pool")
        
                        pooled_outputs.append(pooled)

                # Combine all the pooled features
                num_filters_total = self.num_filters * len(self.filter_sizes)
                self.h_pool = tf.concat(pooled_outputs, 3)
                self.h_pool_flat = tf.reshape(self.h_pool, [-1, num_filters_total])
                # Add dropout
                with tf.name_scope("dropout"):
                    self.h_drop = tf.nn.dropout(self.h_pool_flat, self.config.cnn_dropout)
                
                output = self.h_drop
                # shape = (batch size, max length of sentence, cnn hidden size)
                output = tf.reshape(output, shape=[-1, s[1], int(output.shape[1])])
                
                word_embeddings = tf.concat([word_embeddings,  mor_tag_embeddings, lex_tag_embeddings,output], axis=-1)
        
        self.word_embeddings = tf.nn.dropout(word_embeddings, self.dropout)
    
    def add_logits_op(self):
        """
        Adds logits to self
        """
        with tf.variable_scope("bi-lstm"):
            cell_fw = tf.contrib.rnn.LSTMCell(self.config.hidden_size)
            cell_bw = tf.contrib.rnn.LSTMCell(self.config.hidden_size)
            (output_fw, output_bw), _ = tf.nn.bidirectional_dynamic_rnn(cell_fw,
                                                                        cell_bw, self.word_embeddings,
                                                                        sequence_length=self.sequence_lengths,
                                                                        dtype=tf.float32)
            output = tf.concat([output_fw, output_bw], axis=-1)
            output = tf.nn.dropout(output, self.dropout)
        
        with tf.variable_scope("proj"):
            W = tf.get_variable("W", shape=[2 * self.config.hidden_size, self.ntags],
                                dtype=tf.float32)
            
            b = tf.get_variable("b", shape=[self.ntags], dtype=tf.float32,
                                initializer=tf.zeros_initializer())
            
            ntime_steps = tf.shape(output)[1]
            output = tf.reshape(output, [-1, 2 * self.config.hidden_size])
            pred = tf.matmul(output, W) + b
            self.logits = tf.reshape(pred, [-1, ntime_steps, self.ntags])
    
    def add_pred_op(self):
        """
        Adds labels_pred to self
        """
        if not self.config.crf:
            self.labels_pred = tf.cast(tf.argmax(self.logits, axis=-1), tf.int32)
    
    def add_loss_op(self):
        """
        Adds loss to self
        """
        if self.config.crf:
            log_likelihood, self.transition_params = tf.contrib.crf.crf_log_likelihood(
                self.logits, self.labels, self.sequence_lengths)
            self.loss = tf.reduce_mean(-log_likelihood)
        else:
            losses = tf.nn.sparse_softmax_cross_entropy_with_logits(logits=self.logits, labels=self.labels)
            mask = tf.sequence_mask(self.sequence_lengths)
            losses = tf.boolean_mask(losses, mask)
            self.loss = tf.reduce_mean(losses)
        
        # for tensorboard
        tf.summary.scalar("loss", self.loss)
    
    def add_train_op(self):
        """
        Add train_op to self
        """
        with tf.variable_scope("train_step"):
            # sgd method
            if self.config.lr_method == 'adam':
                optimizer = tf.train.AdamOptimizer(self.lr)
            elif self.config.lr_method == 'adagrad':
                optimizer = tf.train.AdagradOptimizer(self.lr)
            elif self.config.lr_method == 'sgd':
                optimizer = tf.train.GradientDescentOptimizer(self.lr)
            elif self.config.lr_method == 'rmsprop':
                optimizer = tf.train.RMSPropOptimizer(self.lr)
            else:
                raise NotImplementedError("Unknown train op {}".format(
                    self.config.lr_method))
            
            # gradient clipping if config.clip is positive
            if self.config.clip > 0:
                gradients, variables = zip(*optimizer.compute_gradients(self.loss))
                gradients, global_norm = tf.clip_by_global_norm(gradients, self.config.clip)
                self.train_op = optimizer.apply_gradients(zip(gradients, variables))
            else:
                self.train_op = optimizer.minimize(self.loss)
    
    def add_init_op(self):
        self.init = tf.global_variables_initializer()
    
    def add_summary(self, sess):
        # tensorboard stuff
        self.merged = tf.summary.merge_all()
        self.file_writer = tf.summary.FileWriter(self.config.output_path, sess.graph)
    
    def build(self):
        self.add_placeholders()
        self.add_word_embeddings_op()
        self.add_logits_op()
        self.add_pred_op()
        self.add_loss_op()
        self.add_train_op()
        self.add_init_op()
    
    def predict_batch(self, sess, words, mor_tags, lex_tags):
        """
        Args:
            sess: a tensorflow session
            words: list of sentences
        Returns:
            labels_pred: list of labels for each sentence
            sequence_length
        """
        # get the feed dictionnary
        fd, sequence_lengths = self.get_feed_dict(words, mor_tags, lex_tags, dropout=1.0)
        
        if self.config.crf:
            viterbi_sequences = []
            logits, transition_params = sess.run([self.logits, self.transition_params],
                                                 feed_dict=fd)
            # iterate over the sentences
            for logit, sequence_length in zip(logits, sequence_lengths):
                # keep only the valid time steps
                
                logit = logit[:sequence_length]
                viterbi_sequence, viterbi_score = tf.contrib.crf.viterbi_decode(
                    logit, transition_params)
                viterbi_sequences += [viterbi_sequence]
            return viterbi_sequences, sequence_lengths
        
        else:
            labels_pred = sess.run(self.labels_pred, feed_dict=fd)
            return labels_pred, sequence_lengths
    
    def run_epoch(self, sess, train, dev, tags, epoch):
        """
        Performs one complete pass over the train set and evaluate on dev
        Args:
            sess: tensorflow session
            train: dataset that yields tuple of sentences, tags
            dev: dataset
            tags: {tag: index} dictionary
            epoch: (int) number of the epoch
        """
        nbatches = (len(train) + self.config.batch_size - 1) // self.config.batch_size
        prog = Progbar(target=nbatches)
        
        for i, (words, mor_tags, lex_tags, labels) in enumerate(minibatches(train, self.config.batch_size)):
            fd, _ = self.get_feed_dict(words, mor_tags, lex_tags, labels, self.config.lr, self.config.dropout)
            
            _, train_loss, summary = sess.run([self.train_op, self.loss, self.merged], feed_dict=fd)
            
            prog.update(i + 1, [("train loss", train_loss)])
            
            # tensorboard
            if i % 10 == 0:
                self.file_writer.add_summary(summary, epoch * nbatches + i)
        
        acc, f1 = self.run_evaluate(sess, dev, tags)
        self.logger.info("- dev acc {:04.2f} - f1 {:04.2f}".format(100 * acc, 100 * f1))
        return acc, f1
    
    def run_evaluate(self, sess, test, tags):
        """
        Evaluates performance on test set
        Args:
            sess: tensorflow session
            test: dataset that yields tuple of sentences, tags
            tags: {tag: index} dictionary
        Returns:
            accuracy
            f1 score
        """
        accs = []
        correct_preds, total_correct, total_preds = 0., 0., 0.
        
        for words, mor_tags, lex_tags, labels in minibatches(test, self.config.batch_size):
            labels_pred, sequence_lengths = self.predict_batch(sess, words, mor_tags, lex_tags)
            
            for lab, lab_pred, length in zip(labels, labels_pred, sequence_lengths):
                lab = lab[:length]
                lab_pred = lab_pred[:length]
                accs += [a == b for (a, b) in zip(lab, lab_pred)]
                lab_chunks = set(get_chunks(lab, tags))
                lab_pred_chunks = set(get_chunks(lab_pred, tags))
                correct_preds += len(lab_chunks & lab_pred_chunks)
                total_preds += len(lab_pred_chunks)
                total_correct += len(lab_chunks)
        
        p = correct_preds / total_preds if correct_preds > 0 else 0
        r = correct_preds / total_correct if correct_preds > 0 else 0
        f1 = 2 * p * r / (p + r) if correct_preds > 0 else 0
        acc = np.mean(accs)
        return acc, f1
    
    def train(self, train, dev, tags):
        """
        Performs training with early stopping and lr exponential decay

        Args:
            train: dataset that yields tuple of sentences, tags
            dev: dataset
            tags: {tag: index} dictionary
        """
        best_score = 0
        saver = tf.train.Saver()
        # for early stopping
        nepoch_no_imprv = 0
        
        with tf.Session(config=tf.ConfigProto(log_device_placement=True)) as sess:
            sess.run(self.init)
            if self.config.reload:
                self.logger.info("Reloading the latest trained model...")
                saver.restore(sess, self.config.model_output)
            # tensorboard
            self.add_summary(sess)
            for epoch in range(self.config.nepochs):
                self.logger.info("Epoch {:} out of {:}".format(epoch + 1, self.config.nepochs))
                
                acc, f1 = self.run_epoch(sess, train, dev, tags, epoch)
                
                # decay learning rate
                self.config.lr *= self.config.lr_decay
                
                # early stopping and saving best parameters
                if f1 >= best_score:
                    nepoch_no_imprv = 0
                    if not os.path.exists(self.config.model_output):
                        os.makedirs(self.config.model_output)
                    saver.save(sess, self.config.model_output)
                    best_score = f1
                    self.logger.info("- new best score!")
                
                else:
                    nepoch_no_imprv += 1
                    if nepoch_no_imprv >= self.config.nepoch_no_imprv:
                        self.logger.info("- early stopping {} epochs without improvement".format(
                            nepoch_no_imprv))
                        break
    
    def evaluate(self, test, tags):
        saver = tf.train.Saver()
        with tf.Session() as sess:
            self.logger.info("Testing model over test set")
            saver.restore(sess, self.config.model_output)
            acc, f1 = self.run_evaluate(sess, test, tags)
            self.logger.info("- test acc {:04.2f} - f1 {:04.2f}".format(100 * acc, 100 * f1))
    
    def get_mor_result(self, sentence):
        # korea univ morpheme analyzer
        m_command = "cd data/kmat/bin/;./kmat <<<\'" + sentence + "\' 2>/dev/null"
        result = subprocess.check_output(m_command.encode(encoding='cp949', errors='ignore'), shell=True,
                                         executable='/bin/bash')

        mor_name_lists = []
        mor_tags_lists = []

        for each in result.decode(encoding='cp949', errors='ignore').split('\n'):
            if len(each) > 0:
                try:
                    mor_texts = each.split('\t')[1]
                except:
                    print(each)
                mor_results = mor_texts.split('+')
        
                for each_mor in mor_results:
                    mor_name_lists.append(each_mor.split('/')[0])
                    mor_tags_lists.append(each_mor.split('/')[1])
    
        return mor_name_lists, mor_tags_lists
    
    def interactive_shell(self, tags, processing_word, processing_mor_tag, processing_lex_tag):
        
        idx_to_tag = {idx: tag for tag, idx in tags.items()}
        saver = tf.train.Saver()
        
        with tf.Session() as sess:
            saver.restore(sess, self.config.model_output)
            self.logger.info("""
            This is an interactive mode.
            To exit, enter 'exit'.
            """)
            while True:
                try:
                    try:
                        # for python 2
                        sentence = raw_input("input> ")
                    except NameError:
                        # for python 3
                        sentence = input("input> ")

                    # extract mor tags from sentence
                    words_raw, words_mor_tags = self.get_mor_result(sentence)
                    
                    lex_tags = []
                    for word in words_raw:
                        if word in self.lex_dict:
                            lex_tag = self.lex_dict[word]
                            if ',' in lex_tag:
                                one_lexs_str = str(processing_lex_tag(lex_tag.split(',')[0]))
                                two_lexs_str = str(processing_lex_tag(lex_tag.split(',')[1]))
                                lex_tags += [one_lexs_str + ',' + two_lexs_str]
                            else:
                                lex_tags += [processing_lex_tag(lex_tag)]
                        else:
                            lex_tags += [processing_lex_tag(word)]
                    
                    words_raw = [w.strip() for w in words_raw]
                    words_mor_tags = [w.strip() for w in words_mor_tags]
                    
                    if words_raw == ["exit"]:
                        break
                    
                    words = [processing_word(w) for w in words_raw]
                    if type(words[0]) == tuple:
                        words = zip(*words)
                    
                    mor_tags = [processing_mor_tag(w) for w in words_mor_tags]
                    
                    pred_ids, _ = self.predict_batch(sess, [words], [mor_tags], [lex_tags])
                    preds = [idx_to_tag[idx] for idx in list(pred_ids[0])]
                    
                    print('-------------- Predict ----------')
                    for word, pred in zip(words_raw, preds):
                        print(word, pred)
                
                except Exception as e:
                    print('Error : ', e)
                    pass

    def get_ner_tag_result(self, sentence, tags, processing_word, processing_mor_tag, processing_lex_tag):
        idx_to_tag = {idx: tag for tag, idx in tags.items()}
        saver = tf.train.Saver()
        with tf.Session() as sess:
            saver.restore(sess, self.config.model_output)
            # extract mor tags from sentence
            words_raw, words_mor_tags = self.get_mor_result(sentence)

            lex_tags = []
            for word in words_raw:
                if word in self.lex_dict:
                    lex_tag = self.lex_dict[word]
                    if ',' in lex_tag:
                        one_lexs_str = str(processing_lex_tag(lex_tag.split(',')[0]))
                        two_lexs_str = str(processing_lex_tag(lex_tag.split(',')[1]))
                        lex_tags += [one_lexs_str + ',' + two_lexs_str]
                    else:
                        lex_tags += [processing_lex_tag(lex_tag)]
                else:
                    lex_tags += [processing_lex_tag(word)]

            words_raw = [w.strip() for w in words_raw]
            words_mor_tags = [w.strip() for w in words_mor_tags]
            words = [processing_word(w) for w in words_raw]
            if type(words[0]) == tuple:
                words = zip(*words)

            mor_tags = [processing_mor_tag(w) for w in words_mor_tags]

            pred_ids, _ = self.predict_batch(sess, [words], [mor_tags], [lex_tags])
            preds = [idx_to_tag[idx] for idx in list(pred_ids[0])]
            
            return words_raw, preds

    def find_more_than_one_word(self, sentence, search):
        result = re.findall('\\b' + search + '\\b', sentence, flags=re.IGNORECASE)
        
        if len(result) > 1:
            return True
        else:
            return False
    
    def write_tag_result(self, tags, processing_word, processing_mor_tag, processing_lex_tag):
        test_file_name = './data/test_data/test_file'
        tag_result_file_name = './data/test_data/tag_result_file'

        idx_to_tag = {idx: tag for tag, idx in tags.items()}
        saver = tf.train.Saver()
        with tf.Session() as sess:
            saver.restore(sess, self.config.model_output)
            with open(tag_result_file_name, 'w', encoding='utf-8') as f_w:
                with open(test_file_name, 'r', encoding='utf-8') as f_r:
                    for line in f_r:
                        line = line.rstrip()
                        if len(line) < 1:
                            continue
                        if line.startswith(';'):
                            raw_line = line
                            sentence = line.split(';')[1].strip()
                            sentence = sentence.replace("'", "")

                            # extract mor tags from sentence
                            words_raw, words_mor_tags = self.get_mor_result(sentence)
                            
                            lex_tags = []
                            for word in words_raw:
                                if word in self.lex_dict:
                                    lex_tag = self.lex_dict[word]
                                    if ',' in lex_tag:
                                        one_lexs_str = str(processing_lex_tag(lex_tag.split(',')[0]))
                                        two_lexs_str = str(processing_lex_tag(lex_tag.split(',')[1]))
                                        lex_tags += [one_lexs_str + ',' + two_lexs_str]
                                    else:
                                        lex_tags += [processing_lex_tag(lex_tag)]
                                else:
                                    lex_tags += [processing_lex_tag(word)]

                            words_raw = [w.strip() for w in words_raw]
                            words_mor_tags = [w.strip() for w in words_mor_tags]
                            
                            words = [processing_word(w) for w in words_raw]
                            if type(words[0]) == tuple:
                                words = zip(*words)

                            mor_tags = [processing_mor_tag(w) for w in words_mor_tags]

                            pred_ids, _ = self.predict_batch(sess, [words], [mor_tags], [lex_tags])
                            preds = [idx_to_tag[idx] for idx in list(pred_ids[0])]
                            
                            f_w.write('%s\n' % (raw_line))
                            exist_tag_idx_list = [i for i, x in enumerate(preds) if x != 'O']

                            pred_word_list = []
                            pred_tag_list = []
                            
                            prev_word = ''
                            prev_pred = ''
                            dt_count = 0
                            for idx in exist_tag_idx_list:
                                current_pred = preds[idx]
                                current_pred = current_pred.replace('B_', '')
                                current_word = words_raw[idx]
                                if current_pred != 'I':
                                    if prev_word != '':
                                        #f_w.write('%s\t%s\n' % (prev_word, prev_pred))
                                        if dt_count < 4 and prev_pred == 'DT':
                                            prev_word = prev_word.replace(' ', '')
                                        
                                        pred_word_list.append(prev_word)
                                        pred_tag_list.append(prev_pred)
                                        dt_count = 0
                                    prev_word = current_word
                                    prev_pred = current_pred
                                else:
                                    if prev_pred == 'OG':
                                        prev_word = prev_word + current_word
                                    elif prev_pred != 'DT' and prev_pred != 'OG':
                                        # PS, LC, TI
                                        dt_count = 0
                                        prev_word = prev_word + ' ' + current_word
                                    else:
                                        # in case of date time
                                        dt_count += 1
                                        if dt_count == 2:
                                            prev_word = prev_word + ' ' + current_word
                                        else:
                                            prev_word = prev_word + current_word
                                        
                            if prev_word != '':
                                #f_w.write('%s\t%s\n' % (prev_word, prev_pred))
                                if dt_count < 4 and prev_pred == 'DT':
                                    prev_word = prev_word.replace(' ', '')
                                
                                pred_word_list.append(prev_word)
                                pred_tag_list.append(prev_pred)

                            raw_line = raw_line.replace('; ', '$')
                            
                            exist_word_dict = {}
                            for word, pred in zip(pred_word_list, pred_tag_list):
                                if word in exist_word_dict:
                                    continue
                                if self.find_more_than_one_word(raw_line, word):
                                    exist_word_dict[word] = 0
                                #raw_line = re.sub('\\b' + word + '\\b', '<' + word + ':' + pred + '>', raw_line)
                                
                                raw_line = raw_line.replace(word, '<' + word + ':' + pred + '>')
                            f_w.write('%s\n' % (raw_line))
                            
                            for word, pred in zip(pred_word_list, pred_tag_list):
                                if word in exist_word_dict:
                                    if exist_word_dict[word] > 0:
                                        continue
                                    exist_word_dict[word] += 1
                                f_w.write('%s\t%s\n' % (word, pred))
                            f_w.write('\n')