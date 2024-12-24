Use a validator to check the YAML config against the schema, e.g.:

yajsv -s schema.json f355-motronic-5.2.yaml
or
ajv validate -s schema.json -d f355-motronic-5.2.yaml

