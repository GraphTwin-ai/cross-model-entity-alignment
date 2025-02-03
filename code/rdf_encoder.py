from rdflib import Graph
from transformers import BertTokenizer, BertModel
import torch


path = "data/rdf/rdf_triples.ttl"

g = Graph()
g.parse(path, format="turtle")


# Printing the number of triples in the graph
print(len(g))

# Extracting the triples
triples = [(s, p, o) for s, p, o in g]

# Printing the some examples
for s, p, o in triples:
    print(f"Subject: {s}, Predicate: {p}, Object: {o}")


# Tokenizer and model
tokenizer = BertTokenizer.from_pretrained('bert-base-uncased')
model = BertModel.from_pretrained('bert-base-uncased')



