# SPDX-License-Identifier: GPL-3.0-or-later
"""Frozen proposed account-session DATA, not an implemented authentication source."""
from .catalogs import build as catalogs


# Explicit source labels are fixture metadata, never paths to recipe inputs.
PREFIX_FILES = (
    'retained_context_vectors.json',
    'sqlite_receipts_vectors.json',
    'sqlite_contributions_vectors.json',
    'sqlite_application_plans_vectors.json',
    'sqlite_application_bundle_vectors.json',
    'sqlite_application_holds_vectors.json',
    'sqlite_reference_capture_vectors.json',
)

# Existing synthetic verifier/digest/snapshot literals are not computed by this tool.
IDENTITY = 'subathon.sqlite-account-sessions.v1'
CONTRACT_BLOB = '8dc7ba9f'
COORDINATE_STATUS = 'proposed8 literal disposable fixture only; integrator allocates actual production coordinate'
H0 = '$argon2id$v=19$m=4882,t=2,p=1$bA81arsiXysd3WbTRzmEOw$Nm8QBM+7RH1DXo9rvp5cwKEOOOfD2g6JuxlXihoNcpE'
D0_HEX = '0000000000000000000000000000000000000000000000000000000000000000'
D1_HEX = '0000000000000000000000000000000000000000000000000000000000000001'
SCOPE0_SNAPSHOT0_HEX = '5341533100000000000000000000000000000000000000000000000000000000000000000000000000000000'
SCOPE0_SNAPSHOT1_HEX = '5341533100000000000000000000000000000000000000000000000000000000000000000000000000000001'
SOURCE_BYTES = {'complete_session_success': 114,
 'complete_local_success': 152,
 'page_three_success': 240,
 'supplier_strings': 55,
 'snapshot': 44}


# Exact independently supplied source/refusal statements, in original order.
BASE_SQL = (
    (
        "INSERT INTO main.subathon_account_installation VALUES(1,X'49',X'45',X'4a')"
    ),
    (
        "INSERT INTO main.subathon_accounts VALUES(X'49',X'41',1,X'46',X'4b')"
    ),
    (
        'INSERT INTO main.subathon_local_credentials '
        "VALUES(X'49',X'41',X'4c',1,X'246172676f6e32696424763d3139246d3d343838322c743d322c703d312462413831617273695879736433576254527a6d454f77244e6d3851424d2b3752483144586f397276703563774b454f4f4f66443267364a75786c5869686f4e637045')"
    ),
    (
        'INSERT INTO main.subathon_account_sessions '
        "VALUES(X'49',X'41',X'53',X'45',X'46',X'52',1,X'0000000000000000000000000000000000000000000000000000000000000000',0,0,1000,1000,86401000)"
    ),
)

CONSTRAINTS = (
    (
        "INSERT INTO main.subathon_account_installation VALUES(2,X'58',X'45',X'4a')"
    ),
    (
        "INSERT INTO main.subathon_accounts VALUES(X'49',X'42',2,X'46',X'4b')"
    ),
    (
        "INSERT INTO main.subathon_accounts VALUES(X'49',X'42',1,X'',X'4b')"
    ),
    (
        "INSERT INTO main.subathon_accounts VALUES(X'49',X'42',1,NULL,X'4b')"
    ),
    (
        "INSERT INTO main.subathon_accounts VALUES(X'49',12,1,X'46',X'4b')"
    ),
    (
        "INSERT INTO main.subathon_accounts VALUES(X'49',1.5,1,X'46',X'4b')"
    ),
    (
        "INSERT INTO main.subathon_accounts VALUES(X'58',X'42',1,X'46',X'4b')"
    ),
    (
        'UPDATE main.subathon_account_sessions SET '
        "digest='0000000000000000000000000000000000000000000000000000000000000000'"
    ),
    (
        'UPDATE main.subathon_account_sessions SET '
        "digest=X'00000000000000000000000000000000000000000000000000000000000000'"
    ),
    (
        'UPDATE main.subathon_account_sessions SET digest_format=2'
    ),
    (
        'UPDATE main.subathon_account_sessions SET retention=2'
    ),
    (
        'UPDATE main.subathon_account_sessions SET expires_utc_ms=1000'
    ),
    (
        'UPDATE main.subathon_account_sessions SET authenticated_utc_ms=1000.5'
    ),
    (
        'UPDATE main.subathon_account_sessions SET lifetime_started_utc_ms=1001'
    ),
    (
        'UPDATE main.subathon_local_credentials SET verifier_format=2'
    ),
    (
        'UPDATE main.subathon_local_credentials SET encoded_verifier=zeroblob(128)'
    ),
    (
        'UPDATE main.subathon_local_credentials SET account_ref=zeroblob(257)'
    ),
)


def build(seed_root):
    steps = catalogs(seed_root)
    return {
        "identity": IDENTITY,
        "contract_blob": CONTRACT_BLOB,
        "coordinate_status": COORDINATE_STATUS,
        "prefixes": [
            {"fixture": name, "version": step["version"], "id": step["id"],
             "payload_bytes": step["payload_bytes"], "sha256_hex": step["sha256_hex"]}
            for name, step in zip(PREFIX_FILES, steps[:7])
        ],
        "manifest": steps[7],
        "h0": H0,
        "d0_hex": D0_HEX,
        "d1_hex": D1_HEX,
        "scope0_snapshot0_hex": SCOPE0_SNAPSHOT0_HEX,
        "scope0_snapshot1_hex": SCOPE0_SNAPSHOT1_HEX,
        "source_bytes": SOURCE_BYTES,
        "base_sql": list(BASE_SQL),
        "constraints": list(CONSTRAINTS),
    }
