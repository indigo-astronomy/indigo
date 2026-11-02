# Specification

AC-1: PTP scalar/string value polling and Sony propertylists reject truncated device replies before decoding/publishing; preserve valid old/newSony forms and value types.
AC-2: Canon variable image-format entries in value/list events stay within actualeventsize; reject malformed entries without partial invalid publication.
AC-3: Delegated tools close/check IO, validate formats/sizes, avoid shellpath interpretation and maintain coherentbuildlifecycle; approvedgenerator fails safely withoutvalidoutputchange.
AC-4: Delegated driver callbacks/checksums/manualprotocol/motiondeadlines/simulatorbuffer replies follow their existing contracts with bounded failures and portabletests.
AC-5: HTTPrequest/header framing has progress+totaldeadlines without disrupting establishedsessions/resourcecallbacks; blankACL lines safe.

Baseline9557ca85e. PTP own architecture uses shareddecoder plusvendor paths, fakeUSB suite and separateICAtransport; actual replysize must gate USBpadded and ICAexactbuffers alike. ExistingSDKtest fixture supports allvendorprofiles. Hardwaredecision: none. Sourceonlyuntestednetwork/decoderbaseline risks; original safecontrols and syntheticexactlength reproducers beforefix. Highcomplexity/securityboundary, inheritedhostmodel retained and independentreview required. No new publicproperty. InternalPTP decoder API may gainend/size arguments as expressly required byfinding; publicbusABI unchanged.
