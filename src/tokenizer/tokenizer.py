from utils import get_pair_freqs, merge_pair, render_token
import regex as re


COLOR_LIST = [
    '102;194;165', '252;141;98', '141;160;203',
    '231;138;195', '166;216;84', '255;217;47'
]


class CEGATokenize:
    """Tokenizer Class for CEGA"""

    def __init__(self):
        self.merges = {}
        self.special_tokens = {} 
        self.vocab = {}
        self.inverse_special_tokens = {}
        self.pattern = ""

    def train(self, text, vocab_size):
        """given a text and a vocab_sie
        saves merges and vocab lookups that will be used in encode and decode funcs"""

        assert vocab_size >= 256, "vocab_size should be >= 256"

        raw_bytes = text.encode("utf-8")
        ids = list(raw_bytes)

        merges = {} #lookup: pair -> new_token(idx)
        vocab = {idx: bytes([idx]) for idx in range(256)} #lookup: new_token(idx) -> byte
        new_token = 256

        while new_token < vocab_size:
            freqs = get_pair_freqs(ids)
            top_pair = max(freqs, key=freqs.get)
            if freqs[top_pair] == 1: break
            ids = merge_pair(ids, top_pair, new_token)
            merges[top_pair] = new_token
            vocab[new_token] = vocab[top_pair[0]] + vocab[top_pair[1]] #byte concat
            new_token += 1

        #save merges and vocab lookups
        self.merges = merges
        self.vocab = vocab


    def register_special_tokens(self, special_tokens):
        """special_tokens is a lookup of str -> idx
        example: "<|endofrandomwalk|>": 18294"""
        self.special_tokens = special_tokens
        self.inverse_special_tokens = {v: k for k,v in special_tokens.items()}

    def decode(self, ids):
        """given a list of integeres (ids) in [0, vocab_size+len(special_tokens)]
        return python string"""
        if len(self.special_tokens) == 0:
            tokens = b"".join(self.vocab[idx] for idx in ids) #we join because concatenated in training, we let decoding handle this.
            text = tokens.decode("utf-8", errors="replace") #to replace invalid byte seqs with a placeholder char
            return text
        else:
            tokens_bytes = []
            for idx in ids:
                if idx in self.vocab:
                    tokens_bytes.append(self.vocab[idx]) #if regular idx, append raw byte
                elif idx in self.inverse_special_tokens:
                    tokens_bytes.append(self.inverse_special_tokens[idx].encode("utf-8")) #if special token, append utf-8 code of the string

                else:
                    raise ValueError(f"invalid token id: {idx}")
            tokens = b"".join(tokens_bytes)
            text = tokens.decode("utf-8", errors="replace")
            return text
    
    def encode_no_special(self, text):
        tokens = list(text.encode("utf-8"))
        while len(tokens) >= 2: #loops until we break. avoid single token case because causes error inside min(), freqs is empty
            freqs = get_pair_freqs(tokens) #we don't care about the frequencies, we only need the pairs
            pair = min(freqs, key=lambda p: self.merges.get(p, float("inf"))) #get the earliest pair in freqs | merges. get inf if no intersection
            if pair not in self.merges: #the case when no intersection between merges and freqs 
                break #nothing to merge
            tokens = merge_pair(tokens, pair, self.merges[pair])
        return tokens 

    def encode(self, text, include_special=False):
        if not include_special:
            tokens = self.encode_no_special(text)
            return tokens
        
        else:
            tokens = []
            special_pattern = "(" + "|".join(re.escape(k) for k in self.special_tokens) + ")"
            text_chunks = re.split(special_pattern, text)
            for chunk in text_chunks:
                if chunk in self.special_tokens:
                    tokens.append(self.special_tokens[chunk])
                else:
                    tokens.extend(self.encode_no_special(chunk))
            return tokens
        
    def _build_vocab(self):
        # vocab is simply and deterministically derived from merges
        vocab = {idx: bytes([idx]) for idx in range(256)}
        for (p0, p1), idx in self.merges.items():
            vocab[idx] = vocab[p0] + vocab[p1]
        for special, idx in self.special_tokens.items():
            vocab[idx] = special.encode("utf-8")
        return vocab
    
    def save(self, file_prefix, version="v0"):
        """
        Saves two files: file_prefix.vocab and file_prefix.model
        - model file is the critical one, intended for load()
        - vocab file is just a pretty printed version for human inspection only
        """
        model_file = file_prefix + ".model"
        with open(model_file, 'w') as f:
            #write the version
            f.write(f"cegaBBPE {version}\n")
            f.write(f"{self.pattern}\n")
            # write the special tokens, first the number of them, then each one
            f.write(f"{len(self.special_tokens)}\n")
            for special, idx in self.special_tokens.items():
                f.write(f"{special} {idx}\n")
            # the merges dict
            for idx1, idx2 in self.merges:
                f.write(f"{idx1} {idx2}\n")
        # write the vocab: for the human to look at
        vocab_file = file_prefix + ".vocab"
        inverted_merges = {idx: pair for pair, idx in self.merges.items()}
        with open(vocab_file, "w", encoding="utf-8") as f:
            for idx, token in self.vocab.items():
                s = render_token(token)
                # find the children of this token, if any
                if idx in inverted_merges:
                    # if this token has children, render it nicely as a merge
                    idx0, idx1 = inverted_merges[idx]
                    s0 = render_token(self.vocab[idx0])
                    s1 = render_token(self.vocab[idx1])
                    f.write(f"[{s0}][{s1}] -> [{s}] {idx}\n")
                else:
                    # otherwise this is leaf token, just print it
                    # (this should just be the first 256 tokens, the bytes)
                    f.write(f"[{s}] {idx}\n")

    def load(self, model_file):
        """Inverse of save() but only for the model file"""
        assert model_file.endswith(".model")
        # read the model file
        merges = {}
        special_tokens = {}
        idx = 256
        with open(model_file, 'r', encoding="utf-8") as f:
            # read the version
            version = f.readline().strip()
            # read the pattern
            self.pattern = f.readline().strip()
            # read the special tokens
            num_special = int(f.readline().strip())
            for _ in range(num_special):
                special, special_idx = f.readline().strip().split()
                special_tokens[special] = int(special_idx)
            # read the merges
            for line in f:
                idx1, idx2 = map(int, line.split())
                merges[(idx1, idx2)] = idx
                idx += 1
        self.merges = merges
        self.special_tokens = special_tokens
        self.vocab = self._build_vocab()

    def show_tokens(self, text):
        token_ids = self.encode(text)

        for idx, t in enumerate(token_ids):
            color = COLOR_LIST[idx % len(COLOR_LIST)]
            print(
                f'\033[48;2;{color}m' +
                self.decode([t]) +
                '\033[0m',
                end=' '
            )
        print() 
    












        