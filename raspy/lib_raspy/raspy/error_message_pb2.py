# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: error_message.proto

from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
from google.protobuf import descriptor_pb2
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()




DESCRIPTOR = _descriptor.FileDescriptor(
  name='error_message.proto',
  package='rasnet.service',
  # syntax='proto3',
  serialized_pb=b'\n\x13\x65rror_message.proto\x12\x0erasnet.service\"\xb5\x01\n\x0c\x45rrorMessage\x12\x10\n\x08\x65rror_no\x18\x01 \x01(\r\x12\x0c\n\x04kind\x18\x02 \x01(\x05\x12\x12\n\nerror_text\x18\x03 \x01(\t\x12;\n\x04type\x18\x04 \x01(\x0e\x32-.rasnet.service.ErrorMessage.ErrorMessageType\"4\n\x10\x45rrorMessageType\x12\n\n\x06RERROR\x10\x00\x12\x07\n\x03STL\x10\x01\x12\x0b\n\x07UNKNOWN\x10\x02\x42#\n\x1borg.rasdaman.rasnet.service\x80\x01\x00\x88\x01\x00\x62\x06proto3'
)
_sym_db.RegisterFileDescriptor(DESCRIPTOR)



_ERRORMESSAGE_ERRORMESSAGETYPE = _descriptor.EnumDescriptor(
  name='ErrorMessageType',
  full_name='rasnet.service.ErrorMessage.ErrorMessageType',
  filename=None,
  file=DESCRIPTOR,
  values=[
    _descriptor.EnumValueDescriptor(
      name='RERROR', index=0, number=0,
      options=None,
      type=None),
    _descriptor.EnumValueDescriptor(
      name='STL', index=1, number=1,
      options=None,
      type=None),
    _descriptor.EnumValueDescriptor(
      name='UNKNOWN', index=2, number=2,
      options=None,
      type=None),
  ],
  containing_type=None,
  options=None,
  serialized_start=169,
  serialized_end=221,
)
_sym_db.RegisterEnumDescriptor(_ERRORMESSAGE_ERRORMESSAGETYPE)


_ERRORMESSAGE = _descriptor.Descriptor(
  name='ErrorMessage',
  full_name='rasnet.service.ErrorMessage',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='error_no', full_name='rasnet.service.ErrorMessage.error_no', index=0,
      number=1, type=13, cpp_type=3, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='kind', full_name='rasnet.service.ErrorMessage.kind', index=1,
      number=2, type=5, cpp_type=1, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='error_text', full_name='rasnet.service.ErrorMessage.error_text', index=2,
      number=3, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='type', full_name='rasnet.service.ErrorMessage.type', index=3,
      number=4, type=14, cpp_type=8, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
    _ERRORMESSAGE_ERRORMESSAGETYPE,
  ],
  options=None,
  is_extendable=False,
  # syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=40,
  serialized_end=221,
)

_ERRORMESSAGE.fields_by_name['type'].enum_type = _ERRORMESSAGE_ERRORMESSAGETYPE
_ERRORMESSAGE_ERRORMESSAGETYPE.containing_type = _ERRORMESSAGE
DESCRIPTOR.message_types_by_name['ErrorMessage'] = _ERRORMESSAGE

ErrorMessage = _reflection.GeneratedProtocolMessageType('ErrorMessage', (_message.Message,), dict(
  DESCRIPTOR = _ERRORMESSAGE,
  __module__ = 'error_message_pb2'
  # @@protoc_insertion_point(class_scope:rasnet.service.ErrorMessage)
  ))
_sym_db.RegisterMessage(ErrorMessage)


DESCRIPTOR.has_options = True
DESCRIPTOR._options = _descriptor._ParseOptions(descriptor_pb2.FileOptions(), b'\n\033org.rasdaman.rasnet.service\200\001\000\210\001\000')
import abc
from grpc.beta import implementations as beta_implementations
from grpc.early_adopter import implementations as early_adopter_implementations
from grpc.framework.alpha import utilities as alpha_utilities
from grpc.framework.common import cardinality
from grpc.framework.interfaces.face import utilities as face_utilities
# @@protoc_insertion_point(module_scope)
