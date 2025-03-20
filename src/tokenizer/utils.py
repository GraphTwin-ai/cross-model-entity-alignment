


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
        if ids[pos] == pair[0] and ids[pos+1] == pair[1] and pos < len(ids) - 1:
            updated_ids.append(new_token)
            pos += 2 #skip the two merged tokens
        else:
            updated_ids.append(ids[pos])
            pos += 1
    return updated_ids


def train(ids: list, vocab_size: int):
    vocab = list(range(256))
    new_token = 256
    ids = list(ids) 
    merges = {} #to track merged ids
    while new_token < vocab_size:
        freqs = get_pair_freqs(ids)
        top_pair = max(freqs, key=freqs.get)
        if freqs[top_pair] == 1: break
        ids = merge_pair(ids, top_pair, new_token)
        vocab.append(new_token)
        merges[top_pair] = new_token
        new_token += 1

    return vocab, merges


def decode(ids):
    """given a list of integers, return a a python string"""
    
    i = 0
    string = str()
    while i < len(ids):
        b1 = ids[i]
        #if first byte in [0, 127] -> single ASCII byte
        if 0 <= b1 <=127:
            string += chr(b1)
            i += 1 #go to the next integer
        
        #if first byte in [192, 223] -> start of a 2 bytes codepoint
        elif 192 <= b1 <= 223:
            b2 = ids[i+1] #get second byte
            if not (128 <= b2 <= 191):
                i += 2 #invalid sequence, no decoding, skip 2 bytes
            else: 
                codepoint = ((b1 - 192) << 6) | (b2 - 128)
                string += chr(codepoint)
                i += 2 #skip 2 bytes
        
        #if first byte in [224, 239] -> start of a 3 bytes codepoint
        elif 224 <= b1 <= 239:
            b2, b3 = ids[i+1], ids[i+2]
            if not ((128 <= b2 <= 191) and (128 <= b3 <= 191)):
                i += 3 #invalid sequence, no decoding, skip 3 bytes
            else:
                codepoint = ((b1 - 224) << 12) | ((b2 - 128) << 6) | (b3 - 128)
                string += chr(codepoint)
                i += 3 #skip 3 bytes
        
        #if first byte in [240, 247] -> start of a 4 bytes codepoint
        elif 240 <= b1 <= 247:
            b2, b3, b4 = ids[i+1], ids[i+2], ids[i+3]
            if not ((128 <= b2 <= 191) and (128 <= b3 <= 191) and (128 <= b4 <= 191)):
                i += 4 #invalid sequence, no decoding, skip 3 bytes
            else: 
                codepoint = ((b1 - 240) << 18) | ((b2 - 128) << 12) | ((b3 - 128) << 6) | (b4 - 128)
                string += chr(codepoint)
                i += 4
        
        else:
            i += 1 #invalid first byte

    return string
        
    

        
            
def main(txt: str):
    raw_bytes = txt.encode("utf-8")
    tokens = list(map(int, raw_bytes))
    # vocab = train(tokens, vocab_size=vocab_size)
    string = decode(tokens)
    return string



if __name__ == "__main__":
    # ids = [12, 13, 18, 12, 13]
    # freqs = get_pair_freqs(ids)
    # top_pair = max(freqs, key=freqs.get)
    # print(top_pair)
    
    # new_token = 19
    # print(merge_pair(ids, top_pair, new_token))

    # file = open("test_text.txt", "r")
    # txt = file.read()

    # print(main(txt= "aaabdaaabac", vocab_size=270))
    # print(chr((197, 140)))
    print(main("I love food!"))
    

    