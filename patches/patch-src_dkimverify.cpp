$NetBSD: patch-src_dkimverify.cpp,v 1.1 2024/01/31 19:19:58 schmonz Exp $

Fix build with newer OpenSSL (from FreeBSD).

--- src/dkimverify.cpp.orig	2008-09-15 12:21:58.000000000 +0000
+++ src/dkimverify.cpp
@@ -43,8 +43,10 @@ SignatureInfo::SignatureInfo(bool s)
 {
 	VerifiedBodyCount = 0;
 	UnverifiedBodyCount = 0;
-	EVP_MD_CTX_init( &m_Hdr_ctx );
-	EVP_MD_CTX_init( &m_Bdy_ctx );
+	m_Hdr_ctx = EVP_MD_CTX_create();
+	m_Bdy_ctx = EVP_MD_CTX_create();
+	EVP_MD_CTX_init( m_Hdr_ctx );
+	EVP_MD_CTX_init( m_Bdy_ctx );
 	m_pSelector = NULL;
 	Status = DKIM_SUCCESS;
 	m_nHash = 0;
@@ -54,8 +56,8 @@ SignatureInfo::SignatureInfo(bool s)
 
 SignatureInfo::~SignatureInfo()
 {
-	EVP_MD_CTX_cleanup( &m_Hdr_ctx );
-	EVP_MD_CTX_cleanup( &m_Bdy_ctx );
+	EVP_MD_CTX_destroy( m_Hdr_ctx );
+	EVP_MD_CTX_destroy( m_Bdy_ctx );
 }
 
 
@@ -210,7 +212,7 @@ void DecodeQuotedPrintable(char *ptr)
 ////////////////////////////////////////////////////////////////////////////////
 unsigned DecodeBase64(char *ptr)
 {
-	static const unsigned char base64_table[256] = {
+	static const int base64_table[256] = {
 		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
 		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
 		-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
@@ -459,7 +461,7 @@ int CDKIMVerify::GetResults(void)
 				unsigned char md[EVP_MAX_MD_SIZE];
 				unsigned len = 0;
 
-				int res = EVP_DigestFinal( &i->m_Bdy_ctx, md, &len);
+				int res = EVP_DigestFinal( i->m_Bdy_ctx, md, &len);
 
 				if (!res || len != i->BodyHashData.length() || memcmp(i->BodyHashData.data(), md, len) != 0)
 				{
@@ -515,7 +517,7 @@ int CDKIMVerify::GetResults(void)
 
 			assert( i->m_pSelector != NULL );
 
-			int res = EVP_VerifyFinal( &i->m_Hdr_ctx, (unsigned char *) i->SignatureData.data(), i->SignatureData.length(), i->m_pSelector->PublicKey);
+			int res = EVP_VerifyFinal( i->m_Hdr_ctx, (unsigned char *) i->SignatureData.data(), i->SignatureData.length(), i->m_pSelector->PublicKey);
 
 			if (res == 1)
 			{
@@ -658,11 +660,11 @@ void SignatureInfo::Hash( const char* sz
 
 	if (IsBody && !BodyHashData.empty())
 	{
-		EVP_DigestUpdate( &m_Bdy_ctx, szBuffer, nBufLength );
+		EVP_DigestUpdate( m_Bdy_ctx, szBuffer, nBufLength );
 	}
 	else
 	{
-		EVP_VerifyUpdate( &m_Hdr_ctx, szBuffer, nBufLength );
+		EVP_VerifyUpdate( m_Hdr_ctx, szBuffer, nBufLength );
 	}
 
 	if (m_SaveCanonicalizedData)
@@ -741,13 +743,13 @@ int CDKIMVerify::ProcessHeaders(void)
 		// initialize the hashes
 		if (sig.m_nHash == DKIM_HASH_SHA256)
 		{
-			EVP_VerifyInit( &sig.m_Hdr_ctx, EVP_sha256() );
-			EVP_DigestInit( &sig.m_Bdy_ctx, EVP_sha256() );
+			EVP_VerifyInit( sig.m_Hdr_ctx, EVP_sha256() );
+			EVP_DigestInit( sig.m_Bdy_ctx, EVP_sha256() );
 		}
 		else
 		{
-			EVP_VerifyInit( &sig.m_Hdr_ctx, EVP_sha1() );
-			EVP_DigestInit( &sig.m_Bdy_ctx, EVP_sha1() );
+			EVP_VerifyInit( sig.m_Hdr_ctx, EVP_sha1() );
+			EVP_DigestInit( sig.m_Bdy_ctx, EVP_sha1() );
 		}
 
 		// compute the hash of the header
@@ -1337,7 +1339,7 @@ int SelectorInfo::Parse( char* Buffer )
 			return DKIM_SELECTOR_PUBLIC_KEY_INVALID;
 
 		// make sure public key is the correct type (we only support rsa)
-		if (pkey->type == EVP_PKEY_RSA || pkey->type == EVP_PKEY_RSA2)
+		if (EVP_PKEY_id(pkey) == EVP_PKEY_RSA || EVP_PKEY_id(pkey) == EVP_PKEY_RSA2)
 		{
 			PublicKey = pkey;
 		}
