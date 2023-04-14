pub type IContact = *mut ::core::ffi::c_void;
pub type IContactAggregationAggregate = *mut ::core::ffi::c_void;
pub type IContactAggregationAggregateCollection = *mut ::core::ffi::c_void;
pub type IContactAggregationContact = *mut ::core::ffi::c_void;
pub type IContactAggregationContactCollection = *mut ::core::ffi::c_void;
pub type IContactAggregationGroup = *mut ::core::ffi::c_void;
pub type IContactAggregationGroupCollection = *mut ::core::ffi::c_void;
pub type IContactAggregationLink = *mut ::core::ffi::c_void;
pub type IContactAggregationLinkCollection = *mut ::core::ffi::c_void;
pub type IContactAggregationManager = *mut ::core::ffi::c_void;
pub type IContactAggregationServerPerson = *mut ::core::ffi::c_void;
pub type IContactAggregationServerPersonCollection = *mut ::core::ffi::c_void;
pub type IContactCollection = *mut ::core::ffi::c_void;
pub type IContactManager = *mut ::core::ffi::c_void;
pub type IContactProperties = *mut ::core::ffi::c_void;
pub type IContactPropertyCollection = *mut ::core::ffi::c_void;
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CGD_ARRAY_NODE: u32 = 8u32;
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CGD_BINARY_PROPERTY: u32 = 4u32;
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CGD_DATE_PROPERTY: u32 = 2u32;
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CGD_DEFAULT: u32 = 0u32;
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CGD_STRING_PROPERTY: u32 = 1u32;
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CGD_UNKNOWN_PROPERTY: u32 = 0u32;
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CLSID_ContactAggregationManager: ::windows_sys::core::GUID = ::windows_sys::core::GUID::from_u128(0x96c8ad95_c199_44de_b34e_ac33c442df39);
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_PUB_AGENT: ::windows_sys::core::PCWSTR = ::windows_sys::w!("Agent");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_PUB_BBS: ::windows_sys::core::PCWSTR = ::windows_sys::w!("BBS");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_PUB_BUSINESS: ::windows_sys::core::PCWSTR = ::windows_sys::w!("Business");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_PUB_CAR: ::windows_sys::core::PCWSTR = ::windows_sys::w!("Car");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_PUB_CELLULAR: ::windows_sys::core::PCWSTR = ::windows_sys::w!("Cellular");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_PUB_DOMESTIC: ::windows_sys::core::PCWSTR = ::windows_sys::w!("Domestic");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_PUB_FAX: ::windows_sys::core::PCWSTR = ::windows_sys::w!("Fax");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_PUB_INTERNATIONAL: ::windows_sys::core::PCWSTR = ::windows_sys::w!("International");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_PUB_ISDN: ::windows_sys::core::PCWSTR = ::windows_sys::w!("ISDN");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_PUB_LOGO: ::windows_sys::core::PCWSTR = ::windows_sys::w!("Logo");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_PUB_MOBILE: ::windows_sys::core::PCWSTR = ::windows_sys::w!("Mobile");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_PUB_MODEM: ::windows_sys::core::PCWSTR = ::windows_sys::w!("Modem");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_PUB_OTHER: ::windows_sys::core::PCWSTR = ::windows_sys::w!("Other");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_PUB_PAGER: ::windows_sys::core::PCWSTR = ::windows_sys::w!("Pager");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_PUB_PARCEL: ::windows_sys::core::PCWSTR = ::windows_sys::w!("Parcel");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_PUB_PCS: ::windows_sys::core::PCWSTR = ::windows_sys::w!("PCS");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_PUB_PERSONAL: ::windows_sys::core::PCWSTR = ::windows_sys::w!("Personal");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_PUB_POSTAL: ::windows_sys::core::PCWSTR = ::windows_sys::w!("Postal");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_PUB_PREFERRED: ::windows_sys::core::PCWSTR = ::windows_sys::w!("Preferred");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_PUB_TTY: ::windows_sys::core::PCWSTR = ::windows_sys::w!("TTY");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_PUB_USERTILE: ::windows_sys::core::PCWSTR = ::windows_sys::w!("UserTile");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_PUB_VIDEO: ::windows_sys::core::PCWSTR = ::windows_sys::w!("Video");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_PUB_VOICE: ::windows_sys::core::PCWSTR = ::windows_sys::w!("Voice");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_WAB_ANNIVERSARY: ::windows_sys::core::PCWSTR = ::windows_sys::w!("wab:Anniversary");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_WAB_ASSISTANT: ::windows_sys::core::PCWSTR = ::windows_sys::w!("wab:Assistant");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_WAB_BIRTHDAY: ::windows_sys::core::PCWSTR = ::windows_sys::w!("wab:Birthday");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_WAB_CHILD: ::windows_sys::core::PCWSTR = ::windows_sys::w!("wab:Child");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_WAB_MANAGER: ::windows_sys::core::PCWSTR = ::windows_sys::w!("wab:Manager");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_WAB_SCHOOL: ::windows_sys::core::PCWSTR = ::windows_sys::w!("wab:School");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_WAB_SOCIALNETWORK: ::windows_sys::core::PCWSTR = ::windows_sys::w!("wab:SocialNetwork");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_WAB_SPOUSE: ::windows_sys::core::PCWSTR = ::windows_sys::w!("wab:Spouse");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTLABEL_WAB_WISHLIST: ::windows_sys::core::PCWSTR = ::windows_sys::w!("wab:WishList");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_CREATIONDATE: ::windows_sys::core::PCWSTR = ::windows_sys::w!("CreationDate");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_GENDER: ::windows_sys::core::PCWSTR = ::windows_sys::w!("Gender");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_GENDER_FEMALE: ::windows_sys::core::PCWSTR = ::windows_sys::w!("Female");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_GENDER_MALE: ::windows_sys::core::PCWSTR = ::windows_sys::w!("Male");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_GENDER_UNSPECIFIED: ::windows_sys::core::PCWSTR = ::windows_sys::w!("Unspecified");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L1_CERTIFICATECOLLECTION: ::windows_sys::core::PCWSTR = ::windows_sys::w!("CertificateCollection");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L1_CONTACTIDCOLLECTION: ::windows_sys::core::PCWSTR = ::windows_sys::w!("ContactIDCollection");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L1_DATECOLLECTION: ::windows_sys::core::PCWSTR = ::windows_sys::w!("DateCollection");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L1_EMAILADDRESSCOLLECTION: ::windows_sys::core::PCWSTR = ::windows_sys::w!("EmailAddressCollection");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L1_IMADDRESSCOLLECTION: ::windows_sys::core::PCWSTR = ::windows_sys::w!("IMAddressCollection");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L1_NAMECOLLECTION: ::windows_sys::core::PCWSTR = ::windows_sys::w!("NameCollection");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L1_PERSONCOLLECTION: ::windows_sys::core::PCWSTR = ::windows_sys::w!("PersonCollection");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L1_PHONENUMBERCOLLECTION: ::windows_sys::core::PCWSTR = ::windows_sys::w!("PhoneNumberCollection");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L1_PHOTOCOLLECTION: ::windows_sys::core::PCWSTR = ::windows_sys::w!("PhotoCollection");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L1_PHYSICALADDRESSCOLLECTION: ::windows_sys::core::PCWSTR = ::windows_sys::w!("PhysicalAddressCollection");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L1_POSITIONCOLLECTION: ::windows_sys::core::PCWSTR = ::windows_sys::w!("PositionCollection");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L1_URLCOLLECTION: ::windows_sys::core::PCWSTR = ::windows_sys::w!("UrlCollection");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L2_CERTIFICATE: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/Certificate");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L2_CONTACTID: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/ContactID");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L2_DATE: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/Date");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L2_EMAILADDRESS: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/EmailAddress");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L2_IMADDRESSENTRY: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/IMAddress");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L2_NAME: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/Name");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L2_PERSON: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/Person");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L2_PHONENUMBER: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/PhoneNumber");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L2_PHOTO: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/Photo");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L2_PHYSICALADDRESS: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/PhysicalAddress");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L2_POSITION: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/Position");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L2_URL: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/Url");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_ADDRESS: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/Address");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_ADDRESSLABEL: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/AddressLabel");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_ALTERNATE: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/Alternate");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_COMPANY: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/Company");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_COUNTRY: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/Country");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_DEPARTMENT: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/Department");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_EXTENDEDADDRESS: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/ExtendedAddress");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_FAMILYNAME: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/FamilyName");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_FORMATTEDNAME: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/FormattedName");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_GENERATION: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/Generation");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_GIVENNAME: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/GivenName");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_JOB_TITLE: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/JobTitle");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_LOCALITY: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/Locality");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_MIDDLENAME: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/MiddleName");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_NICKNAME: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/NickName");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_NUMBER: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/Number");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_OFFICE: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/Office");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_ORGANIZATION: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/Organization");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_PERSONID: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/PersonID");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_PHONETIC: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/Phonetic");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_POBOX: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/POBox");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_POSTALCODE: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/PostalCode");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_PREFIX: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/Prefix");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_PROFESSION: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/Profession");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_PROTOCOL: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/Protocol");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_REGION: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/Region");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_ROLE: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/Role");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_STREET: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/Street");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_SUFFIX: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/Suffix");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_THUMBPRINT: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/ThumbPrint");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_TITLE: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/Title");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_TYPE: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/Type");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_URL: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/Url");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_L3_VALUE: ::windows_sys::core::PCWSTR = ::windows_sys::w!("/Value");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_MAILER: ::windows_sys::core::PCWSTR = ::windows_sys::w!("Mailer");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_NOTES: ::windows_sys::core::PCWSTR = ::windows_sys::w!("Notes");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CONTACTPROP_PUB_PROGID: ::windows_sys::core::PCWSTR = ::windows_sys::w!("ProgID");
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const Contact: ::windows_sys::core::GUID = ::windows_sys::core::GUID::from_u128(0x61b68808_8eee_4fd1_acb8_3d804c8db056);
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const ContactManager: ::windows_sys::core::GUID = ::windows_sys::core::GUID::from_u128(0x7165c8ab_af88_42bd_86fd_5310b4285a02);
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub type CONTACT_AGGREGATION_COLLECTION_OPTIONS = i32;
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CACO_DEFAULT: CONTACT_AGGREGATION_COLLECTION_OPTIONS = 0i32;
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CACO_INCLUDE_EXTERNAL: CONTACT_AGGREGATION_COLLECTION_OPTIONS = 1i32;
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CACO_EXTERNAL_ONLY: CONTACT_AGGREGATION_COLLECTION_OPTIONS = 2i32;
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub type CONTACT_AGGREGATION_CREATE_OR_OPEN_OPTIONS = i32;
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CA_CREATE_LOCAL: CONTACT_AGGREGATION_CREATE_OR_OPEN_OPTIONS = 0i32;
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub const CA_CREATE_EXTERNAL: CONTACT_AGGREGATION_CREATE_OR_OPEN_OPTIONS = 1i32;
#[repr(C)]
#[doc = "*Required features: `\"Win32_System_Contacts\"`*"]
pub struct CONTACT_AGGREGATION_BLOB {
    pub dwCount: u32,
    pub lpb: *mut u8,
}
impl ::core::marker::Copy for CONTACT_AGGREGATION_BLOB {}
impl ::core::clone::Clone for CONTACT_AGGREGATION_BLOB {
    fn clone(&self) -> Self {
        *self
    }
}
