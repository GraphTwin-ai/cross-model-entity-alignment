# Guide for Transforming RDF to Property Graph (PG) with Neosemantics

This guide will help you transform RDF data into a Neo4j property graph while ensuring data losslessness using the Neosemantics plugin.

## Step 1: Create a New Neo4j Database and Install Neosemantics Plugin

1. **Create a new Neo4j database**: Follow the Neo4j documentation to create a new database.
2. **Install the Neosemantics plugin**: Download and install the Neosemantics (n10s) plugin from the Neo4j plugins repository.

## Step 2: Configure Neosemantics

### Ensure Node Uniqueness

Run the following command to ensure that each node has a unique URI:

```cypher
CREATE CONSTRAINT n10s_unique_uri FOR (r:Resource) REQUIRE r.uri IS UNIQUE;
```

### Initialize Neosemantics Configuration

Configure Neosemantics with the following parameters:

```cypher
CALL n10s.graphconfig.init({
    handleVocabUris: "KEEP", 
    handleMultival: "ARRAY",
    keepLangTag: true,
    keepCustomDataTypes: true,
    typesToLabels: true });
```
### Handle Vocabulary URIs

The `handleVocabUris` parameter determines how vocabulary URIs are managed during the transformation. You can set it to one of the following values:

- **SHORTEN**: This option creates a prefix for each vocabulary URI. For example, the label `http://x.y/Person` becomes `ns0_Person`, with the mapping available on a node containing schema elements:

    ```json
    {
      "identity": 1,
      "labels": [
        "_NsPrefDef"
      ],
      "properties": {
        "xsd": "http://www.w3.org/2001/XMLSchema#",
        "ns0": "http://x.y/"
      },
      "elementId": "4:fe3fafc9-ee08-466e-a12b-29c4bfcd2d4e:1"
    }
    ```

- **IGNORE**: This option completely ignores the vocabulary URIs. For example, the label `http://x.y/Person` becomes `Person`.

- **KEEP**: This option retains the full vocabulary URIs. For example, the label remains `http://x.y/Person`.

Choose the option that best fits your needs for handling vocabulary URIs in your Neo4j property graph.
### Import RDF Data

Import your RDF data into Neo4j. Replace `"file:///path/to/your/data/rdf.ttl"` with the path to your RDF file and `"Turtle"` with the appropriate format if different (e.g., OWL, XML, N-Triples):

```cypher
CALL n10s.rdf.import.fetch("file:///path/to/your/data/rdf.ttl", "Turtle");
```

This command will import the RDF data into your Neo4j database, transforming it into a property graph while preserving the data's integrity.

## Conclusion

By following these steps, you can successfully transform RDF data into a Neo4j property graph using the Neosemantics plugin, ensuring that no data is lost in the process.