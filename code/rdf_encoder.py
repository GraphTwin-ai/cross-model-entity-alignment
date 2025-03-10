import random
import pandas as pd
import random
import torch
import torch.nn as nn
from torch.utils.data import Dataset
from transformers import BertTokenizer, BertModel
import torch.optim as optim
from torch.utils.data import Dataset, DataLoader
from collections import defaultdict


class RDFDataset(Dataset):
    def __init__(self, file_name, random_walks_len=5, random_walk_per_node=5):
        super().__init__()
        self.data = pd.read_csv(file_name, delimiter=",")
        self.rand_len = random_walks_len
        self.rand_walks_per_node = random_walk_per_node
        print(self.data.head())
        print(self.data.columns)

        self.data["subject_clean"] = self.data["subject"].apply(lambda x: x.split("/")[-1])
        self.data["predicate_clean"] = self.data["predicate"].apply(lambda x: x.split("/")[-1])
        self.data["object_clean"] = self.data["object"].apply(lambda x: x.split("/")[-1])

        print(self.data.head())

        self.data_dict = {}
        self.data_random_walks = {}
        self.vocab = {"[PAD]"}
        for _, row in self.data.iterrows():
            subj = row["subject_clean"]
            pred = row["predicate_clean"]
            obj = row["object_clean"]
            self.vocab.add(subj)
            self.vocab.add(pred)
            self.vocab.add(obj)
            
            if subj not in self.data_dict:
                self.data_dict[subj] = {"predicates": [], "objects": []}
                self.data_random_walks[subj] = []
            self.data_dict[subj]["predicates"].append(pred)
            self.data_dict[subj]["objects"].append(obj)
            self.data_random_walks[subj].append((pred, obj))
        
        print(self.data_dict)

        self.tokenizer = BertTokenizer.from_pretrained("bert-base-uncased")
        self.model = BertModel.from_pretrained("bert-base-uncased")
        self.model.eval() 

        self.dataset = []
        for subject, items in self.data_dict.items():
            subj_inputs = self.tokenizer(subject, return_tensors="pt")
            with torch.no_grad():
                subj_embedding = self.model(**subj_inputs).pooler_output.squeeze(0)

            predicate_embeddings = []
            for pred in items["predicates"]:
                pred_inputs = self.tokenizer(pred, return_tensors="pt")
                with torch.no_grad():
                    pred_embedding = self.model(**pred_inputs).pooler_output.squeeze(0) 
                predicate_embeddings.append(pred_embedding)
            if predicate_embeddings:
                pred_tensor = torch.stack(predicate_embeddings)
                pred_tensor = pred_tensor.transpose(0, 1)
            else:
                pred_tensor = torch.empty(self.model.config.hidden_size, 0)

        
            object_embeddings = []
            for obj in items["objects"]:
                obj_inputs = self.tokenizer(obj, return_tensors="pt")
                with torch.no_grad():
                    obj_embedding = self.model(**obj_inputs).pooler_output.squeeze(0) 
                object_embeddings.append(obj_embedding)

            if object_embeddings:
                obj_tensor = torch.stack(object_embeddings)
                obj_tensor = obj_tensor.transpose(0, 1) 
            else:
                obj_tensor = torch.empty(self.model.config.hidden_size, 0)

            self.dataset.append((subj_embedding, pred_tensor, obj_tensor))
        
        print(self.dataset[1][0].shape)
        print(self.dataset[1][1].shape)
        print(self.dataset[1][2].shape)


    def __len__(self):
        return len(self.dataset)

    def __getitem__(self, idx):
        return self.dataset[idx]

    def generate_random_walk(self, subject: str) -> list:
        """Generates a random walk starting from the given subject with proper padding."""
        if subject not in self.data_random_walks:
            raise ValueError(f"Subject '{subject}' not found in the dataset")

        random_walk = [subject]
        
        while len(random_walk) < self.rand_len:
            current_node = random_walk[-1]
            print(current_node)
            print(self.data_random_walks)
            
            if current_node not in self.data_random_walks:
                break
                
            available_edges = self.data_random_walks[current_node]
            if not available_edges:
                break
        
            predicate, obj = random.choice(available_edges)
            
            if len(random_walk) + 2 > self.rand_len:
                break
                
            random_walk.extend([predicate, obj])
            print(random_walk)
        
        padding_needed = self.rand_len - len(random_walk)
        if padding_needed > 0:
            random_walk.extend(['[PAD]'] * padding_needed)

        return random_walk[:self.rand_len]

    def generate_dataset(self) -> list:
        """Generates the full dataset with multiple random walks per node."""
        dataset = []
        
        for subject in self.data_random_walks:
            walks = [self.generate_random_walk(subject) 
                    for _ in range(self.rand_walks_per_node)]
            dataset.extend(walks)
        
        return dataset



class Vocabulary:
    def __init__(self, random_walks):
        self.vocab = defaultdict(int)
        self.itos = {}
        self.stoi = {}

        for walk in random_walks:
            for token in walk:
                self.vocab[token] += 1
                
        self.special_tokens = ['[MASK]', '[PAD]', '[CLS]', '[SEP]', '[UNK]']
        for token in self.special_tokens:
            self.vocab[token] = float('inf')  
            
        self.itos = {i: token for i, token in enumerate(self.vocab.keys())}
        self.stoi = {token: i for i, token in self.itos.items()}
        
        self.unk_index = self.stoi['[UNK]']
        
    def __len__(self):
        return len(self.vocab)

class MLMDataset(Dataset):
    def __init__(self, random_walks, vocab, mask_prob=0.15):
        self.vocab = vocab
        self.data = []
        
        for walk in random_walks:
            indices = [self.vocab.stoi.get(token, self.vocab.unk_index) for token in walk]
            self.data.append(indices)
            
        self.mask_prob = mask_prob
        
    def __len__(self):
        return len(self.data)
    
    def __getitem__(self, idx):
        sequence = self.data[idx]
        
        masked_sequence = []
        labels = []
        
        for token in sequence:
            if random.random() < self.mask_prob:
                # 80% chance to mask, 10% random token, 10% original
                rand = random.random()
                if rand < 0.8:
                    masked_sequence.append(self.vocab.stoi['[MASK]'])
                elif rand < 0.9:
                    masked_sequence.append(random.choice(list(self.vocab.stoi.values())))
                else:
                    masked_sequence.append(token)
                labels.append(token)
            else:
                masked_sequence.append(token)
                labels.append(-100)  
                
        return {
            'input_ids': torch.tensor(masked_sequence),
            'labels': torch.tensor(labels),
            'attention_mask': torch.tensor([1 if token != self.vocab.stoi['[PAD]'] else 0 for token in sequence])
        }

class TransformerMLM(nn.Module):
    def __init__(self, vocab_size, d_model=256, nhead=8, num_layers=6):
        super().__init__()
        self.embedding = nn.Embedding(vocab_size, d_model)
        self.positional_encoding = nn.Parameter(torch.randn(1, 1024, d_model))  
        self.transformer = nn.TransformerEncoder(
            nn.TransformerEncoderLayer(d_model, nhead),
            num_layers
        )
        self.output_layer = nn.Linear(d_model, vocab_size)
        
    def forward(self, input_ids, attention_mask):
        x = self.embedding(input_ids)  
    
        seq_len = x.size(1)
        x = x + self.positional_encoding[:, :seq_len, :]
        
        x = x.permute(1, 0, 2)
    
        x = self.transformer(x, src_key_padding_mask=~attention_mask.bool())

        x = x.permute(1, 0, 2)  
        logits = self.output_layer(x)
        return logits

if __name__ == "__main__":
    
    file_name = "data/rdf/rdf.txt"
    dataset = RDFDataset(file_name)

    random_walks = dataset.generate_dataset()

    vocab = Vocabulary(random_walks)

    dataset = MLMDataset(random_walks, vocab)
    dataloader = DataLoader(dataset, batch_size=32, shuffle=True)

    device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    model = TransformerMLM(len(vocab)).to(device)
    
    optimizer = optim.AdamW(model.parameters(), lr=1e-4)
    criterion = nn.CrossEntropyLoss(ignore_index=-100)
    
    num_epochs = 10
    for epoch in range(num_epochs):
        model.train()
        total_loss = 0
        
        for batch in dataloader:
            inputs = batch['input_ids'].to(device)
            labels = batch['labels'].to(device)
            attention_mask = batch['attention_mask'].to(device)
            
            optimizer.zero_grad()
            
            outputs = model(inputs, attention_mask)
            loss = criterion(outputs.view(-1, outputs.size(-1)), labels.view(-1))
            
            loss.backward()
            optimizer.step()
            
            total_loss += loss.item()
            
        print(f"Epoch {epoch+1} Loss: {total_loss/len(dataloader)}")
    
    