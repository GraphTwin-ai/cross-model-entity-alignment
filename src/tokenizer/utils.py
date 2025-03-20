


def get_pair_freqs(ids: list):
    """computes frequencies for all consecutive pairs
    Inputs: a list of utf8 codes (0 - 255)
    Returns a dictionary with the frequencies of consecutive pairs
    example: ids = [12, 13, 18, 12, 13] -> {(12, 13): 2, (13, 18): 1, (18, 12): 1}"""
    freqs = {}
    for x, y in zip(ids, ids[1:]):
        freqs[(x, y)] = freqs.get((x, y), 0) + 1
    return freqs


def merge_pair(ids, pair, new_token):
    """merges the pair into a single token
    Inputs: a list of tokens, the pair to merge & the new_token
    Returns: updated ids with merged pair (all occurances)
    example: ids = [12, 13, 18, 12, 13], pair = (12, 13), new_token = 19 -> [19, 18, 19]"""
    updated_ids = []
    pos = 0
    while pos < len(ids):
        if pos < len(ids) - 1 and ids[pos] == pair[0] and ids[pos+1] == pair[1]:
            updated_ids.append(new_token)
            pos += 2 #skip the two merged tokens
        else:
            updated_ids.append(ids[pos])
            pos += 1
    return updated_ids


def train(ids: list, vocab_size: int):
    vocab = {idx: bytes([idx]) for idx in range(256)} #raw bytes of 
    new_token = 256
    ids = list(ids) 
    merges = {} #to track merged ids
    while new_token < vocab_size:
        freqs = get_pair_freqs(ids)
        top_pair = max(freqs, key=freqs.get)
        if freqs[top_pair] == 1: break
        ids = merge_pair(ids, top_pair, new_token)
        merges[top_pair] = new_token
        vocab[new_token] = vocab[top_pair[0]] + vocab[top_pair[1]]
        new_token += 1

    return vocab, merges


def decode(ids, vocab):
    """given a list of idx in [0, vocab_size]
    returns the corresponding python string"""
    tokens = b"".join(vocab[idx] for idx in ids)
    text = tokens.decode("utf-8", errors="replace") #to replace invalid byte seqs with a placeholder char
    return text
        
def encode(txt, merges):
    """given a python string
    returns the corresponding list of tokens"""
    tokens = list(txt.encode("utf-8"))
    while len(tokens) >= 2: #loops until we break
        freqs = get_pair_freqs(tokens) #we don't care about the frequencies, we only need the pairs
        pair = min(freqs, key=lambda p: merges.get(p, float("inf"))) #get the earliest pair in freqs | merges. get inf if no intersection
        if pair not in merges: #the case when no intersection between merges and freqs 
            break #nothing to merge
        tokens = merge_pair(tokens, pair, merges[pair])
    return tokens 
            
            
            



        
            
def main():
    
    txt = "I love food, I love pizza!, My name is Mohammed, This is only a test, hahahah, okay, anyways, hay hay"
    raw_bytes = txt.encode("utf-8")
    tokens = list(raw_bytes)
    # print(tokens)
    # tokens = list(map(int, raw_bytes))
    vocab, merges = train(tokens, vocab_size=280)

    tokens = encode("My PC is very slow", merges)
    # print(tokens)

    string = decode(tokens, vocab)
    print(string)
    # return string




if __name__ == "__main__":

    main()   

    