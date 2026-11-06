# Private AFTER-close continuation correction

Root, 6 November 2026; source authoring only, pending different actual review.
The approved 57-row matrix W8/C3 requires native17 after a real successful close.
The existing private `Continuation` only expressed proceed, known-resource5 and
interrupted18. It therefore could not preserve the actual zero close return and
request the matrix's transport refusal. Reporting a synthetic interrupted result
would change the oracle; retrying the closed descriptor would violate ownership.

Add only private `Continuation::transport_refusal` after the existing values.
It means commit the existing ABI17 transport refusal after preserving the real
operation/result and ownership release. No public ABI, result mapping, phase ID,
record layout/count, resource bound or accepted outcome changes. No owner body
or executable test is supplied. Adapter/core source must still receive actual
independent review and the intended baseline before implementation. This narrow
correction is derived from existing matrix W8/C3 and native contract sections5–7;
it does not add a selectable plan or enlarge the public process contract.
