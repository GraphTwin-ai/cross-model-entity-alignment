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
    handleVocabUris: "SHORTEN", 
    handleMultival: "ARRAY",
    keepLangTag: true,
    keepCustomDataTypes: true,
    typesToLabels: true  // Enables RDF class-to-label mapping
});
```

### Import RDF Data

Import your RDF data into Neo4j. Replace `"file:///path/to/your/data/rdf.ttl"` with the path to your RDF file and `"Turtle"` with the appropriate format if different (e.g., OWL, XML, N-Triples):

```cypher
CALL n10s.rdf.import.fetch("file:///path/to/your/data/rdf.ttl", "Turtle");
```

This command will import the RDF data into your Neo4j database, transforming it into a property graph while preserving the data's integrity.

## Conclusion

By following these steps, you can successfully transform RDF data into a Neo4j property graph using the Neosemantics plugin, ensuring that no data is lost in the process.