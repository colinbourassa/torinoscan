## ECU parameter definitions

This directory contains YAML files that each define the location and format of parameters that may be read using the diagnostic protocol for a single ECU. Each file must also contain the name of the protocol itself and the ECU's address byte (which is generally sent as part of the "slow init" sequence for many K-line protocols.)

Different diagnostic protocols provide different mechanisms for accessing parameter data, and these config files attempt to provide a common means of representing them. Each parameter definition in the YAML must be accompanied with an appropriate set of attributes such that the software will be able to request the data from the ECU. Examples:

* Parameters stored in the ECU's RAM need to be described with the `address` and `numbytes` attributes.
* Parameters stored as an indexed value need the `id` attribute instead of `address`, and do *not* need `numbytes` (because the ECU will always return the appropriate amount of bytes to represent the data being requested.)
* Parameters stored in a "snapshot" (i.e. a page of memory that the ECU always sends in its entirety) require the `snapshot` ID as well as `offset` and `numbytes` to extract the appropriate data from within the snapshot memory.

### Use a validator to check the YAML config against the schema, e.g.:

`yajsv -s schema.json f355-motronic-5.2.yaml`

or

`ajv validate -s schema.json -d f355-motronic-5.2.yaml`

### TODO

* There are some aspects of the existing ECU definitions (and JSON schema definition) that are not yet complete. Among these is the format for describing fault code positions, which needs more research.
* Many more ECU definitions can be added.
