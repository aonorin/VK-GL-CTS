/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2015-2016 The Khronos Group Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */ /*!
 * \file
 * \brief
 */ /*-------------------------------------------------------------------*/

/**
 */ /*!
 * \file  gl4cDirectStateAccessTexturesTests.cpp
 * \brief Conformance tests for the Direct State Access feature functionality (Texture access part).
 */ /*-----------------------------------------------------------------------------------------------------------*/

/* Uncomment this if SubImageErrorsTest crashes during negative test of TextureSubImage (negative value width/height/depth passed to the function). */
/* #define TURN_OFF_SUB_IMAGE_ERRORS_TEST_OF_NEGATIVE_WIDTH_HEIGHT_OR_DEPTH */

/* Includes. */
#include "gl4cDirectStateAccessTests.hpp"

#include "deSharedPtr.hpp"

#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluPixelTransfer.hpp"
#include "gluStrUtil.hpp"

#include "tcuFuzzyImageCompare.hpp"
#include "tcuImageCompare.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuSurface.hpp"
#include "tcuTestLog.hpp"

#include "glw.h"
#include "glwFunctions.hpp"

#include <algorithm>
#include <climits>
#include <set>
#include <sstream>
#include <stack>
#include <string>

namespace gl4cts
{
namespace DirectStateAccess
{
namespace Textures
{
/******************************** Creation Test Implementation   ********************************/

/** @brief Creation Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
CreationTest::CreationTest(deqp::Context& context)
	: deqp::TestCase(context, "textures_creation", "Texture Objects Creation Test")
{
	/* Intentionally left blank. */
}

/** @brief Iterate Creation Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult CreationTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	/* Textures' objects */
	static const glw::GLenum texture_targets[] = { GL_TEXTURE_1D,
												   GL_TEXTURE_2D,
												   GL_TEXTURE_3D,
												   GL_TEXTURE_1D_ARRAY,
												   GL_TEXTURE_2D_ARRAY,
												   GL_TEXTURE_RECTANGLE,
												   GL_TEXTURE_CUBE_MAP,
												   GL_TEXTURE_CUBE_MAP_ARRAY,
												   GL_TEXTURE_BUFFER,
												   GL_TEXTURE_2D_MULTISAMPLE,
												   GL_TEXTURE_2D_MULTISAMPLE_ARRAY };
	static const glw::GLuint texture_targets_count = sizeof(texture_targets) / sizeof(texture_targets[0]);
	static const glw::GLuint textures_count		   = 2;

	glw::GLuint textures_legacy[textures_count]						= {};
	glw::GLuint textures_dsa[texture_targets_count][textures_count] = {};

	try
	{
		/* Check legacy state creation. */
		gl.genTextures(textures_count, textures_legacy);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

		for (glw::GLuint i = 0; i < textures_count; ++i)
		{
			if (gl.isTexture(textures_legacy[i]))
			{
				is_ok = false;

				/* Log. */
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "GenTextures has created default objects, but it should create only a names."
					<< tcu::TestLog::EndMessage;
			}
		}

		/* Check direct state creation. */
		for (glw::GLuint j = 0; j < texture_targets_count; ++j)
		{
			gl.createTextures(texture_targets[j], textures_count, textures_dsa[j]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

			for (glw::GLuint i = 0; i < textures_count; ++i)
			{
				if (!gl.isTexture(textures_dsa[j][i]))
				{
					is_ok = false;

					/* Log. */
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "CreateTextures has not created default objects for target "
						<< glu::getTextureTargetStr(texture_targets[j]) << "." << tcu::TestLog::EndMessage;
				}
			}
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	for (glw::GLuint i = 0; i < textures_count; ++i)
	{
		if (textures_legacy[i])
		{
			gl.deleteTextures(1, &textures_legacy[i]);

			textures_legacy[i] = 0;
		}

		for (glw::GLuint j = 0; j < texture_targets_count; ++j)
		{
			if (textures_dsa[j][i])
			{
				gl.deleteTextures(1, &textures_dsa[j][i]);

				textures_dsa[j][i] = 0;
			}
		}
	}

	/* Errors clean up. */
	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/******************************** Reference Data Implementation   *****************************/

/** @brief Internal Format selector.
 *
 *  @tparam T      Type.
 *  @tparam S      Size (# of components).
 *  @tparam N      Is normalized.
 *
 *  @return Internal format.
 */
template <>
glw::GLenum Reference::InternalFormat<glw::GLbyte, 1, false>()
{
	return GL_R8I;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLbyte, 2, false>()
{
	return GL_RG8I;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLbyte, 3, false>()
{
	return GL_RGB8I;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLbyte, 4, false>()
{
	return GL_RGBA8I;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLubyte, 1, false>()
{
	return GL_R8UI;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLubyte, 2, false>()
{
	return GL_RG8UI;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLubyte, 3, false>()
{
	return GL_RGB8UI;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLubyte, 4, false>()
{
	return GL_RGBA8UI;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLshort, 1, false>()
{
	return GL_R16I;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLshort, 2, false>()
{
	return GL_RG16I;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLshort, 3, false>()
{
	return GL_RGB16I;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLshort, 4, false>()
{
	return GL_RGBA16I;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLushort, 1, false>()
{
	return GL_R16UI;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLushort, 2, false>()
{
	return GL_RG16UI;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLushort, 3, false>()
{
	return GL_RGB16UI;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLushort, 4, false>()
{
	return GL_RGBA16UI;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLint, 1, false>()
{
	return GL_R32I;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLint, 2, false>()
{
	return GL_RG32I;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLint, 3, false>()
{
	return GL_RGB32I;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLint, 4, false>()
{
	return GL_RGBA32I;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLuint, 1, false>()
{
	return GL_R32UI;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLuint, 2, false>()
{
	return GL_RG32UI;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLuint, 3, false>()
{
	return GL_RGB32UI;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLuint, 4, false>()
{
	return GL_RGBA32UI;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLubyte, 1, true>()
{
	return GL_R8;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLubyte, 2, true>()
{
	return GL_RG8;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLubyte, 3, true>()
{
	return GL_RGB8;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLubyte, 4, true>()
{
	return GL_RGBA8;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLushort, 1, true>()
{
	return GL_R16;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLushort, 2, true>()
{
	return GL_RG16;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLushort, 3, true>()
{
	return GL_RGB16;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLushort, 4, true>()
{
	return GL_RGBA16;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLfloat, 1, true>()
{
	return GL_R32F;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLfloat, 2, true>()
{
	return GL_RG32F;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLfloat, 3, true>()
{
	return GL_RGB32F;
}

template <>
glw::GLenum Reference::InternalFormat<glw::GLfloat, 4, true>()
{
	return GL_RGBA32F;
}

/** @brief Format selector.
 *
 *  @tparam S      Size (# of components).
 *  @tparam N      Is normalized.
 *
 *  @return format.
 */
template <>
glw::GLenum Reference::Format<1, false>()
{
	return GL_RED_INTEGER;
}

template <>
glw::GLenum Reference::Format<2, false>()
{
	return GL_RG_INTEGER;
}

template <>
glw::GLenum Reference::Format<3, false>()
{
	return GL_RGB_INTEGER;
}

template <>
glw::GLenum Reference::Format<4, false>()
{
	return GL_RGBA_INTEGER;
}

template <>
glw::GLenum Reference::Format<1, true>()
{
	return GL_RED;
}

template <>
glw::GLenum Reference::Format<2, true>()
{
	return GL_RG;
}

template <>
glw::GLenum Reference::Format<3, true>()
{
	return GL_RGB;
}

template <>
glw::GLenum Reference::Format<4, true>()
{
	return GL_RGBA;
}

/** @brief Type selector.
 *
 *  @tparam T      Type.
 *
 *  @return Type.
 */
template <>
glw::GLenum Reference::Type<glw::GLbyte>()
{
	return GL_BYTE;
}

template <>
glw::GLenum Reference::Type<glw::GLubyte>()
{
	return GL_UNSIGNED_BYTE;
}

template <>
glw::GLenum Reference::Type<glw::GLshort>()
{
	return GL_SHORT;
}

template <>
glw::GLenum Reference::Type<glw::GLushort>()
{
	return GL_UNSIGNED_SHORT;
}

template <>
glw::GLenum Reference::Type<glw::GLint>()
{
	return GL_INT;
}

template <>
glw::GLenum Reference::Type<glw::GLuint>()
{
	return GL_UNSIGNED_INT;
}

template <>
glw::GLenum Reference::Type<glw::GLfloat>()
{
	return GL_FLOAT;
}

/** @brief Reference data selector.
 *
 *  @tparam T      Type.
 *  @tparam N      Is normalized.
 *
 *  @return Reference data.
 */

/* RGBA8I */
template <>
const glw::GLbyte* Reference::ReferenceData<glw::GLbyte, false>()
{
	static const glw::GLbyte reference[s_reference_count] = {
		0,  -1,  2,  -3,  4,  -5,  6,  -7,  8,  -9,  10, -11, 12, -13, 14, -15, 16, -17, 18, -19, 20, -21, 22, -23,
		24, -25, 26, -27, 28, -29, 30, -31, 32, -33, 34, -35, 36, -37, 38, -39, 40, -41, 42, -43, 44, -45, 46, -47,
		48, -49, 50, -51, 52, -53, 54, -55, 56, -57, 58, -59, 60, -61, 62, -63, 64, -65, 66, -67, 68, -69, 70, -71,
		72, -73, 74, -75, 76, -77, 78, -79, 80, -81, 82, -83, 84, -85, 86, -87, 88, -89, 90, -91, 92, -93, 94, -95
	};
	return reference;
}

/* RGBA8UI */
template <>
const glw::GLubyte* Reference::ReferenceData<glw::GLubyte, false>()
{
	static const glw::GLubyte reference[s_reference_count] = {
		0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
		24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71,
		72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95
	};
	return reference;
}

/* RGBA16I */
template <>
const glw::GLshort* Reference::ReferenceData<glw::GLshort, false>()
{
	static const glw::GLshort reference[s_reference_count] = {
		0,  -1,  2,  -3,  4,  -5,  6,  -7,  8,  -9,  10, -11, 12, -13, 14, -15, 16, -17, 18, -19, 20, -21, 22, -23,
		24, -25, 26, -27, 28, -29, 30, -31, 32, -33, 34, -35, 36, -37, 38, -39, 40, -41, 42, -43, 44, -45, 46, -47,
		48, -49, 50, -51, 52, -53, 54, -55, 56, -57, 58, -59, 60, -61, 62, -63, 64, -65, 66, -67, 68, -69, 70, -71,
		72, -73, 74, -75, 76, -77, 78, -79, 80, -81, 82, -83, 84, -85, 86, -87, 88, -89, 90, -91, 92, -93, 94, -95
	};
	return reference;
}

/* RGBA16UI */
template <>
const glw::GLushort* Reference::ReferenceData<glw::GLushort, false>()
{
	static const glw::GLushort reference[s_reference_count] = {
		0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
		24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71,
		72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95
	};
	return reference;
}

/* RGBA32I */
template <>
const glw::GLint* Reference::ReferenceData<glw::GLint, false>()
{
	static const glw::GLint reference[s_reference_count] = {
		0,  -1,  2,  -3,  4,  -5,  6,  -7,  8,  -9,  10, -11, 12, -13, 14, -15, 16, -17, 18, -19, 20, -21, 22, -23,
		24, -25, 26, -27, 28, -29, 30, -31, 32, -33, 34, -35, 36, -37, 38, -39, 40, -41, 42, -43, 44, -45, 46, -47,
		48, -49, 50, -51, 52, -53, 54, -55, 56, -57, 58, -59, 60, -61, 62, -63, 64, -65, 66, -67, 68, -69, 70, -71,
		72, -73, 74, -75, 76, -77, 78, -79, 80, -81, 82, -83, 84, -85, 86, -87, 88, -89, 90, -91, 92, -93, 94, -95
	};
	return reference;
}

/* RGBA32UI */
template <>
const glw::GLuint* Reference::ReferenceData<glw::GLuint, false>()
{
	static const glw::GLuint reference[s_reference_count] = {
		0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
		24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71,
		72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95
	};
	return reference;
}

/* RGBA8 */
template <>
const glw::GLubyte* Reference::ReferenceData<glw::GLubyte, true>()
{
	static const glw::GLubyte reference[s_reference_count] = {
		0,   2,   5,   8,   10,  13,  16,  18,  21,  24,  26,  29,  32,  34,  37,  40,  42,  45,  48,  51,
		53,  56,  59,  61,  64,  67,  69,  72,  75,  77,  80,  83,  85,  88,  91,  93,  96,  99,  102, 104,
		107, 110, 112, 115, 118, 120, 123, 126, 128, 131, 134, 136, 139, 142, 144, 147, 150, 153, 155, 158,
		161, 163, 166, 169, 171, 174, 177, 179, 182, 185, 187, 190, 193, 195, 198, 201, 204, 206, 209, 212,
		214, 217, 220, 222, 225, 228, 230, 233, 236, 238, 241, 244, 246, 249, 252, 255
	};
	return reference;
}

/* RGBA16 */
template <>
const glw::GLushort* Reference::ReferenceData<glw::GLushort, true>()
{
	static const glw::GLushort reference[s_reference_count] = {
		0,	 689,   1379,  2069,  2759,  3449,  4139,  4828,  5518,  6208,  6898,  7588,  8278,  8967,  9657,  10347,
		11037, 11727, 12417, 13107, 13796, 14486, 15176, 15866, 16556, 17246, 17935, 18625, 19315, 20005, 20695, 21385,
		22074, 22764, 23454, 24144, 24834, 25524, 26214, 26903, 27593, 28283, 28973, 29663, 30353, 31042, 31732, 32422,
		33112, 33802, 34492, 35181, 35871, 36561, 37251, 37941, 38631, 39321, 40010, 40700, 41390, 42080, 42770, 43460,
		44149, 44839, 45529, 46219, 46909, 47599, 48288, 48978, 49668, 50358, 51048, 51738, 52428, 53117, 53807, 54497,
		55187, 55877, 56567, 57256, 57946, 58636, 59326, 60016, 60706, 61395, 62085, 62775, 63465, 64155, 64845, 65535
	};
	return reference;
}

/* RGBA32F */
template <>
const glw::GLfloat* Reference::ReferenceData<glw::GLfloat, true>()
{
	static const glw::GLfloat reference[s_reference_count] = {
		0.f,		   0.0105263158f, 0.0210526316f, 0.0315789474f, 0.0421052632f, 0.0526315789f,
		0.0631578947f, 0.0736842105f, 0.0842105263f, 0.0947368421f, 0.1052631579f, 0.1157894737f,
		0.1263157895f, 0.1368421053f, 0.1473684211f, 0.1578947368f, 0.1684210526f, 0.1789473684f,
		0.1894736842f, 0.2f,		  0.2105263158f, 0.2210526316f, 0.2315789474f, 0.2421052632f,
		0.2526315789f, 0.2631578947f, 0.2736842105f, 0.2842105263f, 0.2947368421f, 0.3052631579f,
		0.3157894737f, 0.3263157895f, 0.3368421053f, 0.3473684211f, 0.3578947368f, 0.3684210526f,
		0.3789473684f, 0.3894736842f, 0.4f,			 0.4105263158f, 0.4210526316f, 0.4315789474f,
		0.4421052632f, 0.4526315789f, 0.4631578947f, 0.4736842105f, 0.4842105263f, 0.4947368421f,
		0.5052631579f, 0.5157894737f, 0.5263157895f, 0.5368421053f, 0.5473684211f, 0.5578947368f,
		0.5684210526f, 0.5789473684f, 0.5894736842f, 0.6f,			0.6105263158f, 0.6210526316f,
		0.6315789474f, 0.6421052632f, 0.6526315789f, 0.6631578947f, 0.6736842105f, 0.6842105263f,
		0.6947368421f, 0.7052631579f, 0.7157894737f, 0.7263157895f, 0.7368421053f, 0.7473684211f,
		0.7578947368f, 0.7684210526f, 0.7789473684f, 0.7894736842f, 0.8f,		   0.8105263158f,
		0.8210526316f, 0.8315789474f, 0.8421052632f, 0.8526315789f, 0.8631578947f, 0.8736842105f,
		0.8842105263f, 0.8947368421f, 0.9052631579f, 0.9157894737f, 0.9263157895f, 0.9368421053f,
		0.9473684211f, 0.9578947368f, 0.9684210526f, 0.9789473684f, 0.9894736842f, 1.f
	};
	return reference;
}

/* Total number of reference components. */
glw::GLuint Reference::ReferenceDataCount()
{
	return s_reference_count;
}

/* Total number of reference size in basic machine units. */
template <typename T>
glw::GLuint Reference::ReferenceDataSize()
{
	return Reference::ReferenceDataCount() * sizeof(T);
}

/** @brief Comparison function (for floats).
 *
 *  @param [in] a      First element.
 *  @param [in] b      Second element.
 *
 *  @return Comparison result.
 */
template <>
bool Reference::Compare<glw::GLfloat>(const glw::GLfloat a, const glw::GLfloat b)
{
	if (de::abs(a - b) < 1.f / 256.f)
	{
		return true;
	}
	return false;
}

/** @brief Comparison function (integer).
 *
 *  @param [in] a      First element.
 *  @param [in] b      Second element.
 *
 *  @return Comparison result.
 */
template <typename T>
bool Reference::Compare(const T a, const T b)
{
	return a == b;
}

/******************************** Buffer Test Implementation   ********************************/

/** @brief Buffer Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
BufferTest::BufferTest(deqp::Context& context)
	: deqp::TestCase(context, "textures_buffer", "Texture Buffer Objects Test")
	, m_fbo(0)
	, m_rbo(0)
	, m_po(0)
	, m_to(0)
	, m_bo(0)
	, m_vao(0)
{
	/* Intentionally left blank. */
}

/** @brief Count of reference data to be teted.
 *
 *  @tparam S      Size (# of components).
 *
 *  @return Count.
 */
template <glw::GLint S>
glw::GLuint			 BufferTest::TestReferenceDataCount()
{
	return s_fbo_size_x * S;
}

/** @brief Size of reference data to be teted..
 *
 *  @tparam T      Type.
 *  @tparam S      Size (# of components).
 *
 *  @return Size.
 */
template <typename T, glw::GLint S>
glw::GLuint BufferTest::TestReferenceDataSize()
{
	return static_cast<glw::GLint>(TestReferenceDataCount<S>() * sizeof(T));
}

/** @brief Fragment shader part selector.
 *
 *  @tparam T      Type.
 *  @tparam N      Is normalized.
 *
 *  @return Array of characters with source code.
 */
template <>
const glw::GLchar* BufferTest::FragmentShaderDeclaration<glw::GLbyte, false>()
{
	return s_fragment_shader_idecl_lowp;
}

template <>
const glw::GLchar* BufferTest::FragmentShaderDeclaration<glw::GLubyte, false>()
{
	return s_fragment_shader_udecl_lowp;
}

template <>
const glw::GLchar* BufferTest::FragmentShaderDeclaration<glw::GLshort, false>()
{
	return s_fragment_shader_idecl_mediump;
}

template <>
const glw::GLchar* BufferTest::FragmentShaderDeclaration<glw::GLushort, false>()
{
	return s_fragment_shader_udecl_mediump;
}

template <>
const glw::GLchar* BufferTest::FragmentShaderDeclaration<glw::GLint, false>()
{
	return s_fragment_shader_idecl_highp;
}

template <>
const glw::GLchar* BufferTest::FragmentShaderDeclaration<glw::GLuint, false>()
{
	return s_fragment_shader_udecl_highp;
}

template <>
const glw::GLchar* BufferTest::FragmentShaderDeclaration<glw::GLubyte, true>()
{
	return s_fragment_shader_fdecl_lowp;
}

template <>
const glw::GLchar* BufferTest::FragmentShaderDeclaration<glw::GLushort, true>()
{
	return s_fragment_shader_fdecl_mediump;
}

template <>
const glw::GLchar* BufferTest::FragmentShaderDeclaration<glw::GLfloat, true>()
{
	return s_fragment_shader_fdecl_highp;
}

/** @brief Create buffer textuew.
 *
 *  @tparam T      Type.
 *  @tparam S      Size (# of components).
 *  @tparam N      Is normalized.
 *
 *  @param [in] use_range)version       Uses TextureBufferRange instead TextureBuffer.
 *
 *  @return True if succeded, false otherwise.
 */
template <typename T, glw::GLint S, bool N>
bool BufferTest::CreateBufferTexture(bool use_range_version)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Objects creation. */
	gl.genTextures(1, &m_to);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

	gl.bindTexture(GL_TEXTURE_BUFFER, m_to);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

	gl.genBuffers(1, &m_bo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers has failed");

	gl.bindBuffer(GL_TEXTURE_BUFFER, m_bo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");

	/* Data setup. */
	if (use_range_version)
	{
		glw::GLint alignment = 1;

		gl.getIntegerv(GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT, &alignment);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv has failed");

		const glw::GLuint b_offset = alignment;
		const glw::GLuint b_size   = TestReferenceDataSize<T, S>() + b_offset;

		gl.bufferData(GL_TEXTURE_BUFFER, b_size, NULL, GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData has failed");

		gl.bufferSubData(GL_TEXTURE_BUFFER, b_offset, TestReferenceDataSize<T, S>(), ReferenceData<T, N>());
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferSubdata has failed");

		gl.textureBufferRange(m_to, InternalFormat<T, S, N>(), m_bo, b_offset, TestReferenceDataSize<T, S>());
	}
	else
	{
		gl.bufferData(GL_TEXTURE_BUFFER, TestReferenceDataSize<T, S>(), ReferenceData<T, N>(), GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBufferData has failed");

		gl.textureBuffer(m_to, InternalFormat<T, S, N>(), m_bo);
	}

	/* Error checking. */
	glw::GLenum error;

	if (GL_NO_ERROR != (error = gl.getError()))
	{
		/* Log. */
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << (use_range_version ? ("glTextureBufferRange") : ("glTextureBuffer"))
			<< " unexpectedly generated error " << glu::getErrorStr(error) << " during test of internal format "
			<< glu::getTextureFormatStr(InternalFormat<T, S, N>()) << "." << tcu::TestLog::EndMessage;

		CleanBufferTexture();

		return false;
	}

	return true;
}

/** @brief Function prepares framebuffer with internal format color attachment.
 *         Viewport is set up. Content of the framebuffer is cleared.
 *
 *  @note The function may throw if unexpected error has occured.
 *
 *  @return if the framebuffer returned is supported
 */
bool BufferTest::PrepareFramebuffer(const glw::GLenum internal_format)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Prepare framebuffer. */
	gl.genFramebuffers(1, &m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers call failed.");

	gl.genRenderbuffers(1, &m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer call failed.");

	gl.renderbufferStorage(GL_RENDERBUFFER, internal_format, s_fbo_size_x, s_fbo_size_y);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage call failed.");

	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer call failed.");

	if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_UNSUPPORTED)
			return false;
		else
			throw 0;
	}

	gl.viewport(0, 0, s_fbo_size_x, s_fbo_size_y);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport call failed.");

	/* Clear framebuffer's content. */
	gl.clearColor(0.0f, 0.0f, 0.0f, 0.0f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor call failed.");

	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear call failed.");

	return true;
}

/** @brief Create program.
 *
 *  @param [in] variable_declaration    Choose variable declaration of the fragment shader.
 */
void BufferTest::PrepareProgram(const glw::GLchar* variable_declaration)
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	struct Shader
	{
		glw::GLchar const* source[3];
		glw::GLsizei const count;
		glw::GLenum const  type;
		glw::GLuint		   id;
	} shader[] = {
		{ { s_vertex_shader, NULL, NULL }, 1, GL_VERTEX_SHADER, 0 },
		{ { s_fragment_shader_head, variable_declaration, s_fragment_shader_tail }, 3, GL_FRAGMENT_SHADER, 0 }
	};

	glw::GLuint const shader_count = sizeof(shader) / sizeof(shader[0]);

	try
	{
		/* Create program. */
		m_po = gl.createProgram();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram call failed.");

		/* Shader compilation. */

		for (glw::GLuint i = 0; i < shader_count; ++i)
		{
			{
				shader[i].id = gl.createShader(shader[i].type);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader call failed.");

				gl.attachShader(m_po, shader[i].id);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader call failed.");

				gl.shaderSource(shader[i].id, shader[i].count, shader[i].source, NULL);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource call failed.");

				gl.compileShader(shader[i].id);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader call failed.");

				glw::GLint status = GL_FALSE;

				gl.getShaderiv(shader[i].id, GL_COMPILE_STATUS, &status);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv call failed.");

				if (GL_FALSE == status)
				{
					glw::GLint log_size = 0;
					gl.getShaderiv(shader[i].id, GL_INFO_LOG_LENGTH, &log_size);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv call failed.");

					glw::GLchar* log_text = new glw::GLchar[log_size];

					gl.getShaderInfoLog(shader[i].id, log_size, NULL, &log_text[0]);

					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Shader compilation has failed.\n"
						<< "Shader type: " << glu::getShaderTypeStr(shader[i].type) << "\n"
						<< "Shader compilation error log:\n"
						<< log_text << "\n"
						<< "Shader source code:\n"
						<< shader[i].source[0] << (shader[i].source[1] ? shader[i].source[1] : "")
						<< (shader[i].source[2] ? shader[i].source[2] : "") << "\n"
						<< tcu::TestLog::EndMessage;

					delete[] log_text;

					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderInfoLog call failed.");

					throw 0;
				}
			}
		}

		/* Link. */
		gl.linkProgram(m_po);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings call failed.");

		glw::GLint status = GL_FALSE;

		gl.getProgramiv(m_po, GL_LINK_STATUS, &status);

		if (GL_TRUE == status)
		{
			for (glw::GLuint i = 0; i < shader_count; ++i)
			{
				if (shader[i].id)
				{
					gl.detachShader(m_po, shader[i].id);

					GLU_EXPECT_NO_ERROR(gl.getError(), "glDetachShader call failed.");
				}
			}
		}
		else
		{
			glw::GLint log_size = 0;

			gl.getProgramiv(m_po, GL_INFO_LOG_LENGTH, &log_size);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv call failed.");

			glw::GLchar* log_text = new glw::GLchar[log_size];

			gl.getProgramInfoLog(m_po, log_size, NULL, &log_text[0]);

			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Program linkage has failed due to:\n"
												<< log_text << "\n"
												<< tcu::TestLog::EndMessage;

			delete[] log_text;

			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramInfoLog call failed.");

			throw 0;
		}
	}
	catch (...)
	{
		if (m_po)
		{
			gl.deleteProgram(m_po);

			m_po = 0;
		}
	}

	for (glw::GLuint i = 0; i < shader_count; ++i)
	{
		if (0 != shader[i].id)
		{
			gl.deleteShader(shader[i].id);

			shader[i].id = 0;
		}
	}

	if (0 == m_po)
	{
		throw 0;
	}
}

/** @brief Create VAO.
 */
void BufferTest::PrepareVertexArray()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genVertexArrays(1, &m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays has failed");

	gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray has failed");
}

/** @brief Test's draw function.
 */
void BufferTest::Draw()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.useProgram(m_po);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram has failed");

	gl.activeTexture(GL_TEXTURE0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture has failed");

	gl.bindTextureUnit(0, m_to);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture has failed");

	gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays has failed");
}

/** @brief Compre results with the reference.
 *
 *  @tparam T      Type.
 *  @tparam S      Size (# of components).
 *  @tparam N      Is normalized.
 *
 *  @return True if equal, false otherwise.
 */
template <typename T, glw::GLint S, bool N>
bool BufferTest::Check()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Fetching data. */
	std::vector<T> result(TestReferenceDataCount<S>());

	gl.pixelStorei(GL_UNPACK_ALIGNMENT, sizeof(T));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPixelStorei has failed");

	gl.pixelStorei(GL_PACK_ALIGNMENT, sizeof(T));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPixelStorei has failed");

	gl.readnPixels(0, 0, s_fbo_size_x, s_fbo_size_y, Format<S, N>(), Type<T>(), TestReferenceDataSize<T, S>(),
				   (glw::GLvoid*)(&result[0]));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels has failed");

	/* Comparison. */
	bool is_ok = true;

	for (glw::GLuint i = 0; i < TestReferenceDataCount<S>(); ++i)
	{
		if (!Compare<T>(result[i], ReferenceData<T, N>()[i]))
		{
			is_ok = false;

			break;
		}
	}

	return is_ok;
}

/** @brief Test function.
 *
 *  @tparam T      Type.
 *  @tparam S      Size (# of components).
 *  @tparam N      Is normalized.
 *
 *  @param [in] use_range_version   Uses TextureBufferRange instead TextureBuffer.
 *
 *  @return True if succeeded, false otherwise.
 */
template <typename T, glw::GLint S, bool N>
bool BufferTest::Test(bool use_range_version)
{
	/* Setup. */
	if (!PrepareFramebuffer(InternalFormat<T, S, N>()))
	{
		/**
                 * If the framebuffer it not supported, means that the
                 * tested combination is unsupported for this driver,
                 * but allowed to be unsupported by OpenGL spec, so we
                 * just skip.
                 */
		CleanFramebuffer();
		CleanErrors();

		return true;
	}

	if (!CreateBufferTexture<T, S, N>(use_range_version))
	{
		CleanFramebuffer();
		CleanErrors();

		return false;
	}

	/* Action. */
	Draw();

	/* Compare results with reference. */
	bool result = Check<T, S, N>();

	/* Cleanup. */
	CleanFramebuffer();
	CleanBufferTexture();
	CleanErrors();

	/* Pass result. */
	return result;
}

/** @brief Lopp test function over S.
 *
 *  @tparam T      Type.
 *  @tparam S      Size (# of components).
 *  @tparam N      Is normalized.
 *
 *  @param [in] use_range_version   Uses TextureBufferRange instead TextureBuffer.
 *  @param [in] skip_rgb            Skip test of S = 3 (needed for some formats).
 *
 *  @return True if tests succeeded, false otherwise.
 */
template <typename T, bool N>
bool BufferTest::LoopTestOverS(bool use_range_version, bool skip_rgb)
{
	/* Prepare one program per test loop. */
	PrepareProgram(FragmentShaderDeclaration<T, N>());

	/* Run tests. */
	bool result = Test<T, 4, N>(use_range_version);

	if (!skip_rgb)
	{
		result &= Test<T, 3, N>(use_range_version);
	}

	result &= Test<T, 2, N>(use_range_version);
	result &= Test<T, 1, N>(use_range_version);

	/* Cleanup.*/
	CleanProgram();
	CleanErrors();

	/* Pass result. */
	return result;
}

/** @brief Clean GL objects
 */
void BufferTest::CleanBufferTexture()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Texture. */
	if (m_to)
	{
		gl.deleteTextures(1, &m_to);

		m_to = 0;
	}

	/* Texture buffer. */
	if (m_bo)
	{
		gl.deleteBuffers(1, &m_bo);

		m_bo = 0;
	}
}

/** @brief Clean GL objects
 */
void BufferTest::CleanFramebuffer()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Framebuffer. */
	if (m_fbo)
	{
		gl.deleteFramebuffers(1, &m_fbo);

		m_fbo = 0;
	}

	/* Renderbuffer. */
	if (m_rbo)
	{
		gl.deleteRenderbuffers(1, &m_rbo);

		m_rbo = 0;
	}
}

/** @brief Clean GL objects
 */
void BufferTest::CleanProgram()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Program. */
	if (m_po)
	{
		gl.useProgram(0);

		gl.deleteProgram(m_po);

		m_po = 0;
	}
}

/** @brief Clean errors.
 */
void BufferTest::CleanErrors()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Query all errors until GL_NO_ERROR occure. */
	while (GL_NO_ERROR != gl.getError())
		;
}

/** @brief Clean GL objects
 */
void BufferTest::CleanVertexArray()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_vao)
	{
		gl.bindVertexArray(0);

		gl.deleteVertexArrays(1, &m_vao);

		m_vao = 0;
	}
}

/** @brief Iterate Buffer Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult BufferTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		PrepareVertexArray();

		for (glw::GLuint i = 0; i < 2; ++i)
		{
			bool use_range = (i == 1);

			is_ok &= LoopTestOverS<glw::GLbyte, false>(use_range, true);
			is_ok &= LoopTestOverS<glw::GLubyte, false>(use_range, true);
			is_ok &= LoopTestOverS<glw::GLshort, false>(use_range, true);
			is_ok &= LoopTestOverS<glw::GLushort, false>(use_range, true);
			is_ok &= LoopTestOverS<glw::GLint, false>(use_range, false);
			is_ok &= LoopTestOverS<glw::GLuint, false>(use_range, false);

			is_ok &= LoopTestOverS<glw::GLubyte, true>(use_range, true);
			is_ok &= LoopTestOverS<glw::GLushort, true>(use_range, true);
			is_ok &= LoopTestOverS<glw::GLfloat, true>(use_range, false);
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	CleanBufferTexture();
	CleanFramebuffer();
	CleanProgram();
	CleanErrors();
	CleanVertexArray();

	/* Errors clean up. */
	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/* Vertex shader source code. */
const glw::GLchar* BufferTest::s_vertex_shader = "#version 450\n"
												 "\n"
												 "void main()\n"
												 "{\n"
												 "    switch(gl_VertexID)\n"
												 "    {\n"
												 "        case 0:\n"
												 "            gl_Position = vec4(-1.0, 1.0, 0.0, 1.0);\n"
												 "            break;\n"
												 "        case 1:\n"
												 "            gl_Position = vec4( 1.0, 1.0, 0.0, 1.0);\n"
												 "            break;\n"
												 "        case 2:\n"
												 "            gl_Position = vec4(-1.0,-1.0, 0.0, 1.0);\n"
												 "            break;\n"
												 "        case 3:\n"
												 "            gl_Position = vec4( 1.0,-1.0, 0.0, 1.0);\n"
												 "            break;\n"
												 "    }\n"
												 "}\n";

/* Fragment shader source program. */
const glw::GLchar* BufferTest::s_fragment_shader_head = "#version 450\n"
														"\n"
														"layout(pixel_center_integer) in vec4 gl_FragCoord;\n"
														"\n";

const glw::GLchar* BufferTest::s_fragment_shader_fdecl_lowp = "uniform samplerBuffer texture_input;\n"
															  "out     vec4          texture_output;\n";

const glw::GLchar* BufferTest::s_fragment_shader_idecl_lowp = "uniform isamplerBuffer texture_input;\n"
															  "out     ivec4          texture_output;\n";

const glw::GLchar* BufferTest::s_fragment_shader_udecl_lowp = "uniform usamplerBuffer texture_input;\n"
															  "out     uvec4          texture_output;\n";

const glw::GLchar* BufferTest::s_fragment_shader_fdecl_mediump = "uniform samplerBuffer texture_input;\n"
																 "out     vec4          texture_output;\n";

const glw::GLchar* BufferTest::s_fragment_shader_idecl_mediump = "uniform isamplerBuffer texture_input;\n"
																 "out     ivec4          texture_output;\n";

const glw::GLchar* BufferTest::s_fragment_shader_udecl_mediump = "uniform usamplerBuffer texture_input;\n"
																 "out     uvec4          texture_output;\n";

const glw::GLchar* BufferTest::s_fragment_shader_fdecl_highp = "uniform samplerBuffer texture_input;\n"
															   "out     vec4          texture_output;\n";

const glw::GLchar* BufferTest::s_fragment_shader_idecl_highp = "uniform isamplerBuffer texture_input;\n"
															   "out     ivec4          texture_output;\n";

const glw::GLchar* BufferTest::s_fragment_shader_udecl_highp = "uniform usamplerBuffer texture_input;\n"
															   "out     uvec4          texture_output;\n";

const glw::GLchar* BufferTest::s_fragment_shader_tail =
	"\n"
	"void main()\n"
	"{\n"
	"    texture_output = texelFetch(texture_input, int(gl_FragCoord.x));\n"
	"}\n";

/******************************** Storage and SubImage Test Implementation   ********************************/

/** @brief Storage Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
StorageAndSubImageTest::StorageAndSubImageTest(deqp::Context& context)
	: deqp::TestCase(context, "textures_storage_and_subimage", "Texture Storage and SubImage Test")
	, m_fbo(0)
	, m_rbo(0)
	, m_po(0)
	, m_to(0)
	, m_vao(0)
{
	/* Intentionally left blank. */
}

/** @brief Count of reference data to be teted.
 *
 *  @tparam S      Size (# of components).
 *  @tparam D      Texture dimenisons.
 *
 *  @return Count.
 */
template <glw::GLint S, glw::GLuint D>
glw::GLuint StorageAndSubImageTest::TestReferenceDataCount()
{
	return 2 /* 1D */ * ((D > 1) ? 3 : 1) /* 2D */ * ((D > 2) ? 4 : 1) /* 3D */ * S /* components */;
}

/** @brief Size of reference data to be teted.
 *
 *  @tparam T      Type.
 *  @tparam S      Size (# of components).
 *  @tparam D      Texture dimenisons.
 *
 *  @return Size.
 */
template <typename T, glw::GLint S, glw::GLuint D>
glw::GLuint StorageAndSubImageTest::TestReferenceDataSize()
{
	return static_cast<glw::GLint>(TestReferenceDataCount<S, D>() * sizeof(T));
}

/** @brief Height, width or depth of reference data to be teted.
 *
 *  @tparam D      Texture dimenisons.
 *
 *  @return Height, width or depth.
 */
template <>
glw::GLuint StorageAndSubImageTest::TestReferenceDataHeight<2>()
{
	return 3;
}

template <>
glw::GLuint StorageAndSubImageTest::TestReferenceDataHeight<3>()
{
	return 3;
}

template <>
glw::GLuint StorageAndSubImageTest::TestReferenceDataDepth<3>()
{
	return 4;
}

template <glw::GLuint D>
glw::GLuint			  StorageAndSubImageTest::TestReferenceDataWidth()
{
	return 2;
}

template <glw::GLuint D>
glw::GLuint			  StorageAndSubImageTest::TestReferenceDataHeight()
{
	return 1;
}

template <glw::GLuint D>
glw::GLuint			  StorageAndSubImageTest::TestReferenceDataDepth()
{
	return 1;
}

/** @brief Fragment shader declaration selector.
 *
 *  @tparam T      Type.
 *  @tparam N      Normalized flag.
 *  @tparam D      Texture dimenisons.
 *
 *  @return Frgment shader source code part.
 */
template <>
const glw::GLchar* StorageAndSubImageTest::FragmentShaderDeclaration<glw::GLbyte, false, 1>()
{
	return s_fragment_shader_1D_fdecl_lowp;
}

template <>
const glw::GLchar* StorageAndSubImageTest::FragmentShaderDeclaration<glw::GLubyte, false, 1>()
{
	return s_fragment_shader_1D_idecl_lowp;
}

template <>
const glw::GLchar* StorageAndSubImageTest::FragmentShaderDeclaration<glw::GLshort, false, 1>()
{
	return s_fragment_shader_1D_udecl_lowp;
}

template <>
const glw::GLchar* StorageAndSubImageTest::FragmentShaderDeclaration<glw::GLushort, false, 1>()
{
	return s_fragment_shader_1D_fdecl_mediump;
}

template <>
const glw::GLchar* StorageAndSubImageTest::FragmentShaderDeclaration<glw::GLint, false, 1>()
{
	return s_fragment_shader_1D_idecl_mediump;
}

template <>
const glw::GLchar* StorageAndSubImageTest::FragmentShaderDeclaration<glw::GLuint, false, 1>()
{
	return s_fragment_shader_1D_udecl_mediump;
}

template <>
const glw::GLchar* StorageAndSubImageTest::FragmentShaderDeclaration<glw::GLubyte, true, 1>()
{
	return s_fragment_shader_1D_fdecl_highp;
}

template <>
const glw::GLchar* StorageAndSubImageTest::FragmentShaderDeclaration<glw::GLushort, true, 1>()
{
	return s_fragment_shader_1D_idecl_highp;
}

template <>
const glw::GLchar* StorageAndSubImageTest::FragmentShaderDeclaration<glw::GLfloat, true, 1>()
{
	return s_fragment_shader_1D_udecl_highp;
}

template <>
const glw::GLchar* StorageAndSubImageTest::FragmentShaderDeclaration<glw::GLbyte, false, 2>()
{
	return s_fragment_shader_2D_fdecl_lowp;
}

template <>
const glw::GLchar* StorageAndSubImageTest::FragmentShaderDeclaration<glw::GLubyte, false, 2>()
{
	return s_fragment_shader_2D_idecl_lowp;
}

template <>
const glw::GLchar* StorageAndSubImageTest::FragmentShaderDeclaration<glw::GLshort, false, 2>()
{
	return s_fragment_shader_2D_udecl_lowp;
}

template <>
const glw::GLchar* StorageAndSubImageTest::FragmentShaderDeclaration<glw::GLushort, false, 2>()
{
	return s_fragment_shader_2D_fdecl_mediump;
}

template <>
const glw::GLchar* StorageAndSubImageTest::FragmentShaderDeclaration<glw::GLint, false, 2>()
{
	return s_fragment_shader_2D_idecl_mediump;
}

template <>
const glw::GLchar* StorageAndSubImageTest::FragmentShaderDeclaration<glw::GLuint, false, 2>()
{
	return s_fragment_shader_2D_udecl_mediump;
}

template <>
const glw::GLchar* StorageAndSubImageTest::FragmentShaderDeclaration<glw::GLubyte, true, 2>()
{
	return s_fragment_shader_2D_fdecl_highp;
}

template <>
const glw::GLchar* StorageAndSubImageTest::FragmentShaderDeclaration<glw::GLushort, true, 2>()
{
	return s_fragment_shader_2D_idecl_highp;
}

template <>
const glw::GLchar* StorageAndSubImageTest::FragmentShaderDeclaration<glw::GLfloat, true, 2>()
{
	return s_fragment_shader_2D_udecl_highp;
}

template <>
const glw::GLchar* StorageAndSubImageTest::FragmentShaderDeclaration<glw::GLbyte, false, 3>()
{
	return s_fragment_shader_3D_fdecl_lowp;
}

template <>
const glw::GLchar* StorageAndSubImageTest::FragmentShaderDeclaration<glw::GLubyte, false, 3>()
{
	return s_fragment_shader_3D_idecl_lowp;
}

template <>
const glw::GLchar* StorageAndSubImageTest::FragmentShaderDeclaration<glw::GLshort, false, 3>()
{
	return s_fragment_shader_3D_udecl_lowp;
}

template <>
const glw::GLchar* StorageAndSubImageTest::FragmentShaderDeclaration<glw::GLushort, false, 3>()
{
	return s_fragment_shader_3D_fdecl_mediump;
}

template <>
const glw::GLchar* StorageAndSubImageTest::FragmentShaderDeclaration<glw::GLint, false, 3>()
{
	return s_fragment_shader_3D_idecl_mediump;
}

template <>
const glw::GLchar* StorageAndSubImageTest::FragmentShaderDeclaration<glw::GLuint, false, 3>()
{
	return s_fragment_shader_3D_udecl_mediump;
}

template <>
const glw::GLchar* StorageAndSubImageTest::FragmentShaderDeclaration<glw::GLubyte, true, 3>()
{
	return s_fragment_shader_3D_fdecl_highp;
}

template <>
const glw::GLchar* StorageAndSubImageTest::FragmentShaderDeclaration<glw::GLushort, true, 3>()
{
	return s_fragment_shader_3D_idecl_highp;
}

template <>
const glw::GLchar* StorageAndSubImageTest::FragmentShaderDeclaration<glw::GLfloat, true, 3>()
{
	return s_fragment_shader_3D_udecl_highp;
}

/** @brief Fragment shader tail selector.
 *
 *  @tparam D      Texture dimenisons.
 *
 *  @return Frgment shader source code part.
 */
template <>
const glw::GLchar* StorageAndSubImageTest::FragmentShaderTail<1>()
{
	return s_fragment_shader_1D_tail;
}

template <>
const glw::GLchar* StorageAndSubImageTest::FragmentShaderTail<2>()
{
	return s_fragment_shader_2D_tail;
}

template <>
const glw::GLchar* StorageAndSubImageTest::FragmentShaderTail<3>()
{
	return s_fragment_shader_3D_tail;
}

/** @brief Texture target selector.
 *
 *  @tparam D      Texture dimenisons.
 *
 *  @return Texture target.
 */
template <>
glw::GLenum StorageAndSubImageTest::TextureTarget<1>()
{
	return GL_TEXTURE_1D;
}

template <>
glw::GLenum StorageAndSubImageTest::TextureTarget<2>()
{
	return GL_TEXTURE_2D;
}

template <>
glw::GLenum StorageAndSubImageTest::TextureTarget<3>()
{
	return GL_TEXTURE_3D;
}

/** @brief TextureStorage* wrapper.
 *
 *  @tparam D      Texture dimenisons.
 *  @tparam I      Select between legacy/DSA way.
 *
 *  @return true if suuceed (in legacy always or throw), false otherwise.
 */
template <>
bool StorageAndSubImageTest::TextureStorage<1, true>(glw::GLenum target, glw::GLuint texture, glw::GLsizei levels,
													 glw::GLenum internalformat, glw::GLsizei width,
													 glw::GLsizei height, glw::GLsizei depth)
{
	(void)texture;
	(void)height;
	(void)depth;
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Preparing storage. */
	gl.texStorage1D(target, levels, internalformat, width);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage1D has failed");

	/* TextureSubImage* (not TextureStorage*) is tested so here we only throw when error occurs. */
	return true;
}

/** @brief TextureStorage* wrapper.
 *
 *  @tparam D      Texture dimenisons.
 *  @tparam I      Select between legacy/DSA way.
 *
 *  @return true if suuceed (in legacy always or throw), false otherwise.
 */
template <>
bool StorageAndSubImageTest::TextureStorage<2, true>(glw::GLenum target, glw::GLuint texture, glw::GLsizei levels,
													 glw::GLenum internalformat, glw::GLsizei width,
													 glw::GLsizei height, glw::GLsizei depth)
{
	(void)texture;
	(void)depth;
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Preparing storage. */
	gl.texStorage2D(target, levels, internalformat, width, height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D has failed");

	/* TextureSubImage* (not TextureStorage*) is tested so here we only throw when error occurs. */
	return true;
}

/** @brief TextureStorage* wrapper.
 *
 *  @tparam D      Texture dimenisons.
 *  @tparam I      Select between legacy/DSA way.
 *
 *  @return true if suuceed (in legacy always or throw), false otherwise.
 */
template <>
bool StorageAndSubImageTest::TextureStorage<3, true>(glw::GLenum target, glw::GLuint texture, glw::GLsizei levels,
													 glw::GLenum internalformat, glw::GLsizei width,
													 glw::GLsizei height, glw::GLsizei depth)
{
	(void)texture;
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Preparing storage. */
	gl.texStorage3D(target, levels, internalformat, width, height, depth);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage3D has failed");

	/* TextureSubImage* (not TextureStorage*) is tested so here we only throw when error occurs. */
	return true;
}

/** @brief TextureStorage* wrapper.
 *
 *  @tparam D      Texture dimenisons.
 *  @tparam I      Select between legacy/DSA way.
 *
 *  @return true if suuceed (in legacy always or throw), false otherwise.
 */
template <>
bool StorageAndSubImageTest::TextureStorage<1, false>(glw::GLenum target, glw::GLuint texture, glw::GLsizei levels,
													  glw::GLenum internalformat, glw::GLsizei width,
													  glw::GLsizei height, glw::GLsizei depth)
{
	(void)target;
	(void)height;
	(void)depth;
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Preparing storage. */
	gl.textureStorage1D(texture, levels, internalformat, width);

	/* Error checking. */
	glw::GLenum error;

	if (GL_NO_ERROR != (error = gl.getError()))
	{
		/* Log. */
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "glTextureStorage1D unexpectedly generated error " << glu::getErrorStr(error)
			<< " during test with levels " << levels << ", internal format " << internalformat << " and width " << width
			<< "." << tcu::TestLog::EndMessage;

		CleanTexture();
		CleanErrors();

		return false;
	}

	return true;
}

/** @brief TextureStorage* wrapper.
 *
 *  @tparam D      Texture dimenisons.
 *  @tparam I      Select between legacy/DSA way.
 *
 *  @return true if suuceed (in legacy always or throw), false otherwise.
 */
template <>
bool StorageAndSubImageTest::TextureStorage<2, false>(glw::GLenum target, glw::GLuint texture, glw::GLsizei levels,
													  glw::GLenum internalformat, glw::GLsizei width,
													  glw::GLsizei height, glw::GLsizei depth)
{
	(void)target;
	(void)depth;
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Preparing storage. */
	gl.textureStorage2D(texture, levels, internalformat, width, height);

	/* Error checking. */
	glw::GLenum error;

	if (GL_NO_ERROR != (error = gl.getError()))
	{
		/* Log. */
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "glTextureStorage2D unexpectedly generated error " << glu::getErrorStr(error)
			<< " during test with levels " << levels << ", internal format " << internalformat << ", width " << width
			<< " and height " << height << "." << tcu::TestLog::EndMessage;

		CleanTexture();
		CleanErrors();

		return false;
	}

	return true;
}

/** @brief TextureStorage* wrapper.
 *
 *  @tparam D      Texture dimenisons.
 *  @tparam I      Select between legacy/DSA way.
 *
 *  @return true if suuceed (in legacy always or throw), false otherwise.
 */
template <>
bool StorageAndSubImageTest::TextureStorage<3, false>(glw::GLenum target, glw::GLuint texture, glw::GLsizei levels,
													  glw::GLenum internalformat, glw::GLsizei width,
													  glw::GLsizei height, glw::GLsizei depth)
{
	(void)target;
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Preparing storage. */
	gl.textureStorage3D(texture, levels, internalformat, width, height, depth);

	/* Error checking. */
	glw::GLenum error;

	if (GL_NO_ERROR != (error = gl.getError()))
	{
		/* Log. */
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "glTextureStorage3D unexpectedly generated error " << glu::getErrorStr(error)
			<< " during test with levels " << levels << ", internal format " << glu::getTextureFormatStr(internalformat)
			<< ", width " << width << ", height " << height << " and depth " << depth << "."
			<< tcu::TestLog::EndMessage;

		CleanTexture();
		CleanErrors();

		return false;
	}

	return true;
}

/** @brief TextureSubImage* wrapper.
 *
 *  @tparam D      Texture dimenisons.
 *  @tparam I      Select between legacy/DSA way.
 *
 *  @return true if suuceed (in legacy always or throw), false otherwise.
 */
template <>
bool StorageAndSubImageTest::TextureSubImage<1, false>(glw::GLenum target, glw::GLuint texture, glw::GLint level,
													   glw::GLint internalformat, glw::GLsizei width,
													   glw::GLsizei height, glw::GLsizei depth, glw::GLenum format,
													   glw::GLenum type, const glw::GLvoid* data)
{
	(void)texture;
	(void)internalformat;
	(void)height;
	(void)depth;
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Action. */
	gl.texSubImage1D(target, level, 0, width, format, type, data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexSubImage1D has failed");

	/* TextureStorage* (not TextureSubImage) is tested so here we only throw when error occurs. */
	return true;
}

/** @brief TextureSubImage* wrapper.
 *
 *  @tparam D      Texture dimenisons.
 *  @tparam I      Select between legacy/DSA way.
 *
 *  @return true if suuceed (in legacy always or throw), false otherwise.
 */
template <>
bool StorageAndSubImageTest::TextureSubImage<2, false>(glw::GLenum target, glw::GLuint texture, glw::GLint level,
													   glw::GLint internalformat, glw::GLsizei width,
													   glw::GLsizei height, glw::GLsizei depth, glw::GLenum format,
													   glw::GLenum type, const glw::GLvoid* data)
{
	(void)texture;
	(void)internalformat;
	(void)depth;
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Action. */
	gl.texSubImage2D(target, level, 0, 0, width, height, format, type, data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexSubImage2D has failed");

	/* TextureStorage* (not TextureSubImage) is tested so here we only throw when error occurs. */
	return true;
}

/** @brief TextureSubImage* wrapper.
 *
 *  @tparam D      Texture dimenisons.
 *  @tparam I      Select between legacy/DSA way.
 *
 *  @return true if suuceed (in legacy always or throw), false otherwise.
 */
template <>
bool StorageAndSubImageTest::TextureSubImage<3, false>(glw::GLenum target, glw::GLuint texture, glw::GLint level,
													   glw::GLint internalformat, glw::GLsizei width,
													   glw::GLsizei height, glw::GLsizei depth, glw::GLenum format,
													   glw::GLenum type, const glw::GLvoid* data)
{
	(void)texture;
	(void)internalformat;
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Action. */
	gl.texSubImage3D(target, level, 0, 0, 0, width, height, depth, format, type, data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexSubImage3D has failed");

	/* TextureStorage* (not TextureSubImage) is tested so here we only throw when error occurs. */
	return true;
}

/** @brief TextureSubImage* wrapper.
 *
 *  @tparam D      Texture dimenisons.
 *  @tparam I      Select between legacy/DSA way.
 *
 *  @return true if suuceed (in legacy always or throw), false otherwise.
 */
template <>
bool StorageAndSubImageTest::TextureSubImage<1, true>(glw::GLenum target, glw::GLuint texture, glw::GLint level,
													  glw::GLint internalformat, glw::GLsizei width,
													  glw::GLsizei height, glw::GLsizei depth, glw::GLenum format,
													  glw::GLenum type, const glw::GLvoid* data)
{
	(void)target;
	(void)internalformat;
	(void)height;
	(void)depth;
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Action. */
	gl.textureSubImage1D(texture, level, 0, width, format, type, data);

	/* Error checking. */
	glw::GLenum error;

	if (GL_NO_ERROR != (error = gl.getError()))
	{
		/* Log. */
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "glTextureSubImage1D unexpectedly generated error " << glu::getErrorStr(error)
			<< " during test with level " << level << ", width " << width << ", format "
			<< glu::getTextureFormatStr(format) << " and type " << glu::getTypeStr(type) << "."
			<< tcu::TestLog::EndMessage;

		CleanTexture();
		CleanErrors();

		return false;
	}

	return true;
}

/** @brief TextureSubImage* wrapper.
 *
 *  @tparam D      Texture dimenisons.
 *  @tparam I      Select between legacy/DSA way.
 *
 *  @return true if suuceed (in legacy always or throw), false otherwise.
 */
template <>
bool StorageAndSubImageTest::TextureSubImage<2, true>(glw::GLenum target, glw::GLuint texture, glw::GLint level,
													  glw::GLint internalformat, glw::GLsizei width,
													  glw::GLsizei height, glw::GLsizei depth, glw::GLenum format,
													  glw::GLenum type, const glw::GLvoid* data)
{
	(void)target;
	(void)internalformat;
	(void)depth;
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Action. */
	gl.textureSubImage2D(texture, level, 0, 0, width, height, format, type, data);

	/* Error checking. */
	glw::GLenum error;

	if (GL_NO_ERROR != (error = gl.getError()))
	{
		/* Log. */
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "glTextureSubImage2D unexpectedly generated error " << glu::getErrorStr(error)
			<< " during test with level " << level << ", width " << width << ", height " << height << ", format "
			<< glu::getTextureFormatStr(format) << " and type " << glu::getTypeStr(type) << "."
			<< tcu::TestLog::EndMessage;

		CleanTexture();
		CleanErrors();

		return false;
	}

	return true;
}

/** @brief TextureSubImage* wrapper.
 *
 *  @tparam D      Texture dimenisons.
 *  @tparam I      Select between legacy/DSA way.
 *
 *  @return true if suuceed (in legacy always or throw), false otherwise.
 */
template <>
bool StorageAndSubImageTest::TextureSubImage<3, true>(glw::GLenum target, glw::GLuint texture, glw::GLint level,
													  glw::GLint internalformat, glw::GLsizei width,
													  glw::GLsizei height, glw::GLsizei depth, glw::GLenum format,
													  glw::GLenum type, const glw::GLvoid* data)
{
	(void)target;
	(void)internalformat;
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Action. */
	gl.textureSubImage3D(texture, level, 0, 0, 0, width, height, depth, format, type, data);

	/* Error checking. */
	glw::GLenum error;

	if (GL_NO_ERROR != (error = gl.getError()))
	{
		/* Log. */
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "glTextureSubImage3D unexpectedly generated error " << glu::getErrorStr(error)
			<< " during test with level " << level << ", width " << width << ", height " << height << ", depth "
			<< depth << ", format " << glu::getTextureFormatStr(format) << " and type " << glu::getTypeStr(type) << "."
			<< tcu::TestLog::EndMessage;

		CleanTexture();
		CleanErrors();

		return false;
	}

	return true;
}

/** @brief Create texture.
 *
 *  @tparam T      Type.
 *  @tparam S      Size (# of components).
 *  @tparam N      Is normalized.
 *  @tparam D      Dimmensions.
 *  @tparam I      Test SubImage or Storage.
 *
 *  @return True if succeded, false otherwise.
 */
template <typename T, glw::GLint S, bool N, glw::GLuint D, bool I>
bool StorageAndSubImageTest::CreateTexture()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Objects creation. */
	gl.genTextures(1, &m_to);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

	gl.bindTexture(TextureTarget<D>(), m_to);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

	/* Storage creation. */
	if (TextureStorage<D, I>(TextureTarget<D>(), m_to, 1, InternalFormat<T, S, N>(), TestReferenceDataWidth<D>(),
							 TestReferenceDataHeight<D>(), TestReferenceDataDepth<D>()))
	{
		/* Data setup. */
		if (TextureSubImage<D, I>(TextureTarget<D>(), m_to, 0, InternalFormat<T, S, N>(), TestReferenceDataWidth<D>(),
								  TestReferenceDataHeight<D>(), TestReferenceDataDepth<D>(), Format<S, N>(), Type<T>(),
								  ReferenceData<T, N>()))
		{
			return true;
		}
	}

	CleanTexture();

	return false;
}

/** @brief Compre results with the reference.
 *
 *  @tparam T      Type.
 *  @tparam S      Size (# of components).
 *  @tparam N      Is normalized.
 *
 *  @return True if equal, false otherwise.
 */
template <typename T, glw::GLint S, bool N, glw::GLuint D>
bool StorageAndSubImageTest::Check()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Fetching data. */
	std::vector<T> result(TestReferenceDataCount<S, D>());

	gl.pixelStorei(GL_UNPACK_ALIGNMENT, sizeof(T));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPixelStorei has failed");

	gl.pixelStorei(GL_PACK_ALIGNMENT, sizeof(T));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPixelStorei has failed");

	glw::GLuint fbo_size_x = 0;

	switch (D)
	{
	case 1:
		fbo_size_x = 2;
		break;
	case 2:
		fbo_size_x = 2 * 3;
		break;
	case 3:
		fbo_size_x = 2 * 3 * 4;
		break;
	default:
		throw 0;
	}

	gl.readnPixels(0, 0, fbo_size_x, 1, Format<S, N>(), Type<T>(), TestReferenceDataSize<T, S, D>(),
				   (glw::GLvoid*)(&result[0]));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels has failed");

	/* Comparison. */
	for (glw::GLuint i = 0; i < TestReferenceDataCount<S, D>(); ++i)
	{
		if (!Compare<T>(result[i], ReferenceData<T, N>()[i]))
		{
			return false;
		}
	}

	return true;
}

/** @brief Test case function.
 *
 *  @tparam T       Type.
 *  @tparam S       Size (# of components).
 *  @tparam N       Is normalized.
 *  @tparam D       Number of texture dimensions.
 *
 *  @return True if test succeeded, false otherwise.
 */
template <typename T, glw::GLint S, bool N, glw::GLuint D, bool I>
bool StorageAndSubImageTest::Test()
{
	/* Setup. */
	PrepareFramebuffer(InternalFormat<T, S, N>(), D);

	if (!CreateTexture<T, S, N, D, I>())
	{
		return false;
	}

	/* Action. */
	Draw();

	/* Compare results with reference. */
	bool result = Check<T, S, N, D>();

	/* Cleanup. */
	CleanTexture();
	CleanFramebuffer();
	CleanErrors();

	/* Pass result. */
	return result;
}

/** @brief Lopp test function over S.
 *
 *  @tparam T      Type.
 *  @tparam N      Is normalized.
 *  @tparam D      Texture dimension.
 *  @tparam I      Choose between SubImage and Storage tests.
 *
 *  @param [in] skip_rgb            Skip test of S = 3 (needed for some formats).
 *
 *  @return True if tests succeeded, false otherwise.
 */
template <typename T, bool N, glw::GLuint D, bool I>
bool StorageAndSubImageTest::LoopTestOverS(bool skip_rgb)
{
	/* Prepare one program per test loop. */
	PrepareProgram(FragmentShaderDeclaration<T, N, D>(), FragmentShaderTail<D>());

	/* Run tests. */
	bool result = true;

	result &= Test<T, 1, N, D, I>();

	result &= Test<T, 2, N, D, I>();

	if (!skip_rgb)
	{
		result &= Test<T, 3, N, D, I>();
	}

	result &= Test<T, 4, N, D, I>();

	/* Cleanup.*/
	CleanProgram();
	CleanErrors();

	/* Pass result. */
	return result;
}

/** @brief Lopp test function over D and over S.
 *
 *  @tparam T      Type.
 *  @tparam N      Is normalized.
 *  @tparam I      Choose between SubImage and Storage tests.
 *
 *  @param [in] skip_rgb            Skip test of S = 3 (needed for some formats).
 *
 *  @return True if tests succeeded, false otherwise.
 */
template <typename T, bool N, bool I>
bool StorageAndSubImageTest::LoopTestOverDOverS(bool skip_rgb)
{
	bool result = true;

	result &= LoopTestOverS<T, N, 1, I>(skip_rgb);
	result &= LoopTestOverS<T, N, 2, I>(skip_rgb);
	result &= LoopTestOverS<T, N, 3, I>(skip_rgb);

	return result;
}

/** @brief Function prepares framebuffer with internal format color attachment.
 *         Viewport is set up. Content of the framebuffer is cleared.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void StorageAndSubImageTest::PrepareFramebuffer(const glw::GLenum internal_format, const glw::GLuint D)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Prepare framebuffer. */
	gl.genFramebuffers(1, &m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers call failed.");

	gl.genRenderbuffers(1, &m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer call failed.");

	glw::GLuint fbo_size_x = 0;

	switch (D)
	{
	case 1:
		fbo_size_x = 2;
		break;
	case 2:
		fbo_size_x = 2 * 3;
		break;
	case 3:
		fbo_size_x = 2 * 3 * 4;
		break;
	default:
		throw 0;
	}

	gl.renderbufferStorage(GL_RENDERBUFFER, internal_format, fbo_size_x, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage call failed.");

	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer call failed.");

	if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		throw 0;
	}

	gl.viewport(0, 0, fbo_size_x, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport call failed.");

	/* Clear framebuffer's content. */
	gl.clearColor(0.0f, 0.0f, 0.0f, 0.0f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor call failed.");

	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear call failed.");
}

/** @brief Prepare program
 *
 *  @param [in] variable_declaration      Variables declaration part of fragment shader source code.
 *  @param [in] tail                      Tail part of fragment shader source code.
 */
void StorageAndSubImageTest::PrepareProgram(const glw::GLchar* variable_declaration, const glw::GLchar* tail)
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	struct Shader
	{
		glw::GLchar const* source[3];
		glw::GLsizei const count;
		glw::GLenum const  type;
		glw::GLuint		   id;
	} shader[] = { { { s_vertex_shader, NULL, NULL }, 1, GL_VERTEX_SHADER, 0 },
				   { { s_fragment_shader_head, variable_declaration, tail }, 3, GL_FRAGMENT_SHADER, 0 } };

	glw::GLuint const shader_count = sizeof(shader) / sizeof(shader[0]);

	try
	{
		/* Create program. */
		m_po = gl.createProgram();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram call failed.");

		/* Shader compilation. */

		for (glw::GLuint i = 0; i < shader_count; ++i)
		{
			{
				shader[i].id = gl.createShader(shader[i].type);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader call failed.");

				gl.attachShader(m_po, shader[i].id);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader call failed.");

				gl.shaderSource(shader[i].id, shader[i].count, shader[i].source, NULL);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource call failed.");

				gl.compileShader(shader[i].id);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader call failed.");

				glw::GLint status = GL_FALSE;

				gl.getShaderiv(shader[i].id, GL_COMPILE_STATUS, &status);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv call failed.");

				if (GL_FALSE == status)
				{
					glw::GLint log_size = 0;
					gl.getShaderiv(shader[i].id, GL_INFO_LOG_LENGTH, &log_size);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv call failed.");

					glw::GLchar* log_text = new glw::GLchar[log_size];

					gl.getShaderInfoLog(shader[i].id, log_size, NULL, &log_text[0]);

					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Shader compilation has failed.\n"
						<< "Shader type: " << glu::getShaderTypeStr(shader[i].type) << "\n"
						<< "Shader compilation error log:\n"
						<< log_text << "\n"
						<< "Shader source code:\n"
						<< shader[i].source[0] << (shader[i].source[1] ? shader[i].source[1] : "")
						<< (shader[i].source[2] ? shader[i].source[2] : "") << "\n"
						<< tcu::TestLog::EndMessage;

					delete[] log_text;

					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderInfoLog call failed.");

					throw 0;
				}
			}
		}

		/* Link. */
		gl.linkProgram(m_po);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings call failed.");

		glw::GLint status = GL_FALSE;

		gl.getProgramiv(m_po, GL_LINK_STATUS, &status);

		if (GL_TRUE == status)
		{
			for (glw::GLuint i = 0; i < shader_count; ++i)
			{
				if (shader[i].id)
				{
					gl.detachShader(m_po, shader[i].id);

					GLU_EXPECT_NO_ERROR(gl.getError(), "glDetachShader call failed.");
				}
			}
		}
		else
		{
			glw::GLint log_size = 0;

			gl.getProgramiv(m_po, GL_INFO_LOG_LENGTH, &log_size);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv call failed.");

			glw::GLchar* log_text = new glw::GLchar[log_size];

			gl.getProgramInfoLog(m_po, log_size, NULL, &log_text[0]);

			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Program linkage has failed due to:\n"
												<< log_text << "\n"
												<< tcu::TestLog::EndMessage;

			delete[] log_text;

			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramInfoLog call failed.");

			throw 0;
		}
	}
	catch (...)
	{
		if (m_po)
		{
			gl.deleteProgram(m_po);

			m_po = 0;
		}
	}

	for (glw::GLuint i = 0; i < shader_count; ++i)
	{
		if (0 != shader[i].id)
		{
			gl.deleteShader(shader[i].id);

			shader[i].id = 0;
		}
	}

	if (0 == m_po)
	{
		throw 0;
	}
}

/** @brief Prepare VAO.
 */
void StorageAndSubImageTest::PrepareVertexArray()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genVertexArrays(1, &m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays has failed");

	gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray has failed");
}

/** @brief Test's draw call.
 */
void StorageAndSubImageTest::Draw()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.useProgram(m_po);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram has failed");

	gl.activeTexture(GL_TEXTURE0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture has failed");

	gl.bindTextureUnit(0, m_to);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture has failed");

	gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays has failed");
}

/** @brief Clean GL objects, test variables and GL errors.
 */
void StorageAndSubImageTest::CleanTexture()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Texture. */
	if (m_to)
	{
		gl.deleteTextures(1, &m_to);

		m_to = 0;
	}
}

/** @brief Clean GL objects, test variables and GL errors.
 */
void StorageAndSubImageTest::CleanFramebuffer()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Framebuffer. */
	if (m_fbo)
	{
		gl.deleteFramebuffers(1, &m_fbo);

		m_fbo = 0;
	}

	/* Renderbuffer. */
	if (m_rbo)
	{
		gl.deleteRenderbuffers(1, &m_rbo);

		m_rbo = 0;
	}
}

/** @brief Clean GL objects, test variables and GL errors.
 */
void StorageAndSubImageTest::CleanProgram()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Program. */
	if (m_po)
	{
		gl.useProgram(0);

		gl.deleteProgram(m_po);

		m_po = 0;
	}
}

/** @brief Clean GL objects, test variables and GL errors.
 */
void StorageAndSubImageTest::CleanErrors()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Query all errors until GL_NO_ERROR occure. */
	while (GL_NO_ERROR != gl.getError())
		;
}

/** @brief Clean GL objects, test variables and GL errors.
 */
void StorageAndSubImageTest::CleanVertexArray()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_vao)
	{
		gl.bindVertexArray(0);

		gl.deleteVertexArrays(1, &m_vao);

		m_vao = 0;
	}
}

/** @brief Iterate Storage Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult StorageAndSubImageTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		PrepareVertexArray();

		/* Test TextureStorage* */
		is_ok &= LoopTestOverDOverS<glw::GLbyte, false, false>(true);
		is_ok &= LoopTestOverDOverS<glw::GLubyte, false, false>(true);
		is_ok &= LoopTestOverDOverS<glw::GLshort, false, false>(true);
		is_ok &= LoopTestOverDOverS<glw::GLushort, false, false>(true);
		is_ok &= LoopTestOverDOverS<glw::GLint, false, false>(false);
		is_ok &= LoopTestOverDOverS<glw::GLuint, false, false>(false);
		is_ok &= LoopTestOverDOverS<glw::GLubyte, true, false>(true);
		is_ok &= LoopTestOverDOverS<glw::GLushort, true, false>(true);
		is_ok &= LoopTestOverDOverS<glw::GLfloat, true, false>(false);

		/* Test TextureSubImage* */
		is_ok &= LoopTestOverDOverS<glw::GLbyte, false, true>(true);
		is_ok &= LoopTestOverDOverS<glw::GLubyte, false, true>(true);
		is_ok &= LoopTestOverDOverS<glw::GLshort, false, true>(true);
		is_ok &= LoopTestOverDOverS<glw::GLushort, false, true>(true);
		is_ok &= LoopTestOverDOverS<glw::GLint, false, true>(false);
		is_ok &= LoopTestOverDOverS<glw::GLuint, false, true>(false);
		is_ok &= LoopTestOverDOverS<glw::GLubyte, true, true>(true);
		is_ok &= LoopTestOverDOverS<glw::GLushort, true, true>(true);
		is_ok &= LoopTestOverDOverS<glw::GLfloat, true, true>(false);
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	CleanTexture();
	CleanFramebuffer();
	CleanProgram();
	CleanErrors();
	CleanVertexArray();

	/* Errors clean up. */
	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/* Vertex shader source code. */
const glw::GLchar* StorageAndSubImageTest::s_vertex_shader = "#version 450\n"
															 "\n"
															 "void main()\n"
															 "{\n"
															 "    switch(gl_VertexID)\n"
															 "    {\n"
															 "        case 0:\n"
															 "            gl_Position = vec4(-1.0, 1.0, 0.0, 1.0);\n"
															 "            break;\n"
															 "        case 1:\n"
															 "            gl_Position = vec4( 1.0, 1.0, 0.0, 1.0);\n"
															 "            break;\n"
															 "        case 2:\n"
															 "            gl_Position = vec4(-1.0,-1.0, 0.0, 1.0);\n"
															 "            break;\n"
															 "        case 3:\n"
															 "            gl_Position = vec4( 1.0,-1.0, 0.0, 1.0);\n"
															 "            break;\n"
															 "    }\n"
															 "}\n";

/* Fragment shader source program. */
const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_head =
	"#version 450\n"
	"\n"
	"layout(pixel_center_integer) in vec4 gl_FragCoord;\n"
	"\n";

const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_1D_fdecl_lowp =
	"uniform  sampler1D texture_input;\nout     vec4          texture_output;\n";
const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_1D_idecl_lowp =
	"uniform isampler1D texture_input;\nout     ivec4         texture_output;\n";
const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_1D_udecl_lowp =
	"uniform usampler1D texture_input;\nout     uvec4         texture_output;\n";
const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_1D_fdecl_mediump =
	"uniform  sampler1D texture_input;\nout     vec4          texture_output;\n";
const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_1D_idecl_mediump =
	"uniform isampler1D texture_input;\nout     ivec4         texture_output;\n";
const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_1D_udecl_mediump =
	"uniform usampler1D texture_input;\nout     uvec4         texture_output;\n";
const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_1D_fdecl_highp =
	"uniform  sampler1D texture_input;\nout     vec4          texture_output;\n";
const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_1D_idecl_highp =
	"uniform isampler1D texture_input;\nout     ivec4         texture_output;\n";
const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_1D_udecl_highp =
	"uniform usampler1D texture_input;\nout     uvec4         texture_output;\n";

const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_2D_fdecl_lowp =
	"uniform  sampler2D texture_input;\nout     vec4          texture_output;\n";
const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_2D_idecl_lowp =
	"uniform isampler2D texture_input;\nout     ivec4         texture_output;\n";
const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_2D_udecl_lowp =
	"uniform usampler2D texture_input;\nout     uvec4         texture_output;\n";
const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_2D_fdecl_mediump =
	"uniform  sampler2D texture_input;\nout     vec4          texture_output;\n";
const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_2D_idecl_mediump =
	"uniform isampler2D texture_input;\nout     ivec4         texture_output;\n";
const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_2D_udecl_mediump =
	"uniform usampler2D texture_input;\nout     uvec4         texture_output;\n";
const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_2D_fdecl_highp =
	"uniform  sampler2D texture_input;\nout     vec4          texture_output;\n";
const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_2D_idecl_highp =
	"uniform isampler2D texture_input;\nout     ivec4         texture_output;\n";
const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_2D_udecl_highp =
	"uniform usampler2D texture_input;\nout     uvec4         texture_output;\n";

const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_3D_fdecl_lowp =
	"uniform  sampler3D texture_input;\nout     vec4          texture_output;\n";
const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_3D_idecl_lowp =
	"uniform isampler3D texture_input;\nout     ivec4         texture_output;\n";
const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_3D_udecl_lowp =
	"uniform usampler3D texture_input;\nout     uvec4         texture_output;\n";
const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_3D_fdecl_mediump =
	"uniform  sampler3D texture_input;\nout     vec4          texture_output;\n";
const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_3D_idecl_mediump =
	"uniform isampler3D texture_input;\nout     ivec4         texture_output;\n";
const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_3D_udecl_mediump =
	"uniform usampler3D texture_input;\nout     uvec4         texture_output;\n";
const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_3D_fdecl_highp =
	"uniform  sampler3D texture_input;\nout     vec4          texture_output;\n";
const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_3D_idecl_highp =
	"uniform isampler3D texture_input;\nout     ivec4         texture_output;\n";
const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_3D_udecl_highp =
	"uniform usampler3D texture_input;\nout     uvec4         texture_output;\n";

const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_1D_tail =
	"\n"
	"void main()\n"
	"{\n"
	"    texture_output = texelFetch(texture_input, int(gl_FragCoord.x), 0);\n"
	"}\n";

const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_2D_tail =
	"\n"
	"void main()\n"
	"{\n"
	"    texture_output = texelFetch(texture_input, ivec2(int(gl_FragCoord.x) % 2, int(floor(gl_FragCoord.x / 2))), "
	"0);\n"
	"}\n";

const glw::GLchar* StorageAndSubImageTest::s_fragment_shader_3D_tail =
	"\n"
	"void main()\n"
	"{\n"
	"    texture_output = texelFetch(texture_input, ivec3(int(gl_FragCoord.x) % 2, int(floor(gl_FragCoord.x / 2)) % 3, "
	"int(floor(gl_FragCoord.x / 2 / 3))), 0);\n"
	"}\n";

/******************************** Storage Multisample Test Implementation   ********************************/

/** @brief Storage Multisample Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
StorageMultisampleTest::StorageMultisampleTest(deqp::Context& context)
	: deqp::TestCase(context, "textures_storage_multisample", "Texture Storage Multisample Test")
	, m_fbo_ms(0)
	, m_fbo_aux(0)
	, m_to_ms(0)
	, m_po_ms(0)
	, m_po_aux(0)
	, m_to(0)
	, m_to_aux(0)
	, m_vao(0)
{
	/* Intentionally left blank. */
}

/** @brief Count of reference data to be teted.
 *
 *  @tparam S      Size (# of components).
 *  @tparam D      Texture dimenisons.
 *
 *  @return Count.
 */
template <glw::GLint S, glw::GLuint D>
glw::GLuint StorageMultisampleTest::TestReferenceDataCount()
{
	return 2 /* 1D */ * ((D > 1) ? 3 : 1) /* 2D */ * ((D > 2) ? 4 : 1) /* 3D */ * S /* components */;
}

/** @brief Size of reference data to be teted.
 *
 *  @tparam T      Type.
 *  @tparam S      Size (# of components).
 *  @tparam D      Texture dimenisons.
 *
 *  @return Size.
 */
template <typename T, glw::GLint S, glw::GLuint D>
glw::GLuint StorageMultisampleTest::TestReferenceDataSize()
{
	return TestReferenceDataCount<S, D>() * sizeof(T);
}

/** @brief Height, width or depth of reference data to be teted.
 *
 *  @tparam D      Texture dimenisons.
 *
 *  @return Height, width or depth.
 */
template <>
glw::GLuint StorageMultisampleTest::TestReferenceDataHeight<2>()
{
	return 3;
}

template <>
glw::GLuint StorageMultisampleTest::TestReferenceDataHeight<3>()
{
	return TestReferenceDataHeight<2>();
}

template <>
glw::GLuint StorageMultisampleTest::TestReferenceDataDepth<2>()
{
	return 1;
}

template <>
glw::GLuint StorageMultisampleTest::TestReferenceDataDepth<3>()
{
	return 4;
}

template <glw::GLuint D>
glw::GLuint			  StorageMultisampleTest::TestReferenceDataWidth()
{
	return 2;
}

/** @brief Fragment shader declaration selector.
 *
 *  @tparam T      Type.
 *  @tparam N      Normalized flag.
 *  @tparam D      Texture dimenisons.
 *
 *  @return Frgment shader source code part.
 */
template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationMultisample<glw::GLubyte, true, 2>()
{
	return s_fragment_shader_ms_2D_fdecl_lowp;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationMultisample<glw::GLbyte, false, 2>()
{
	return s_fragment_shader_ms_2D_idecl_lowp;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationMultisample<glw::GLubyte, false, 2>()
{
	return s_fragment_shader_ms_2D_udecl_lowp;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationMultisample<glw::GLushort, true, 2>()
{
	return s_fragment_shader_ms_2D_fdecl_mediump;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationMultisample<glw::GLshort, false, 2>()
{
	return s_fragment_shader_ms_2D_idecl_mediump;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationMultisample<glw::GLushort, false, 2>()
{
	return s_fragment_shader_ms_2D_udecl_mediump;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationMultisample<glw::GLfloat, true, 2>()
{
	return s_fragment_shader_ms_2D_fdecl_highp;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationMultisample<glw::GLint, false, 2>()
{
	return s_fragment_shader_ms_2D_idecl_highp;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationMultisample<glw::GLuint, false, 2>()
{
	return s_fragment_shader_ms_2D_udecl_highp;
}

/** @brief Fragment shader declaration selector.
 *
 *  @tparam T      Type.
 *  @tparam N      Normalized flag.
 *  @tparam D      Texture dimenisons.
 *
 *  @return Frgment shader source code part.
 */
template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationMultisample<glw::GLubyte, true, 3>()
{
	return s_fragment_shader_ms_3D_fdecl_lowp;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationMultisample<glw::GLbyte, false, 3>()
{
	return s_fragment_shader_ms_3D_idecl_lowp;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationMultisample<glw::GLubyte, false, 3>()
{
	return s_fragment_shader_ms_3D_udecl_lowp;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationMultisample<glw::GLushort, true, 3>()
{
	return s_fragment_shader_ms_3D_fdecl_mediump;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationMultisample<glw::GLshort, false, 3>()
{
	return s_fragment_shader_ms_3D_idecl_mediump;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationMultisample<glw::GLushort, false, 3>()
{
	return s_fragment_shader_ms_3D_udecl_mediump;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationMultisample<glw::GLfloat, true, 3>()
{
	return s_fragment_shader_ms_3D_fdecl_highp;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationMultisample<glw::GLint, false, 3>()
{
	return s_fragment_shader_ms_3D_idecl_highp;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationMultisample<glw::GLuint, false, 3>()
{
	return s_fragment_shader_ms_3D_udecl_highp;
}

/** @brief Fragment shader declaration selector.
 *
 *  @tparam T      Type.
 *  @tparam N      Normalized flag.
 *  @tparam D      Texture dimenisons.
 *
 *  @return Frgment shader source code part.
 */
template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationAuxiliary<glw::GLubyte, true, 2>()
{
	return s_fragment_shader_aux_2D_fdecl_lowp;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationAuxiliary<glw::GLbyte, false, 2>()
{
	return s_fragment_shader_aux_2D_idecl_lowp;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationAuxiliary<glw::GLubyte, false, 2>()
{
	return s_fragment_shader_aux_2D_udecl_lowp;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationAuxiliary<glw::GLushort, true, 2>()
{
	return s_fragment_shader_aux_2D_fdecl_mediump;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationAuxiliary<glw::GLshort, false, 2>()
{
	return s_fragment_shader_aux_2D_idecl_mediump;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationAuxiliary<glw::GLushort, false, 2>()
{
	return s_fragment_shader_aux_2D_udecl_mediump;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationAuxiliary<glw::GLfloat, true, 2>()
{
	return s_fragment_shader_aux_2D_fdecl_highp;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationAuxiliary<glw::GLint, false, 2>()
{
	return s_fragment_shader_aux_2D_idecl_highp;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationAuxiliary<glw::GLuint, false, 2>()
{
	return s_fragment_shader_aux_2D_udecl_highp;
}

/** @brief Fragment shader declaration selector.
 *
 *  @tparam T      Type.
 *  @tparam N      Normalized flag.
 *  @tparam D      Texture dimenisons.
 *
 *  @return Frgment shader source code part.
 */
template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationAuxiliary<glw::GLubyte, true, 3>()
{
	return s_fragment_shader_aux_3D_fdecl_lowp;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationAuxiliary<glw::GLbyte, false, 3>()
{
	return s_fragment_shader_aux_3D_idecl_lowp;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationAuxiliary<glw::GLubyte, false, 3>()
{
	return s_fragment_shader_aux_3D_udecl_lowp;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationAuxiliary<glw::GLushort, true, 3>()
{
	return s_fragment_shader_aux_3D_fdecl_mediump;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationAuxiliary<glw::GLshort, false, 3>()
{
	return s_fragment_shader_aux_3D_idecl_mediump;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationAuxiliary<glw::GLushort, false, 3>()
{
	return s_fragment_shader_aux_3D_udecl_mediump;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationAuxiliary<glw::GLfloat, true, 3>()
{
	return s_fragment_shader_aux_3D_fdecl_highp;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationAuxiliary<glw::GLint, false, 3>()
{
	return s_fragment_shader_aux_3D_idecl_highp;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderDeclarationAuxiliary<glw::GLuint, false, 3>()
{
	return s_fragment_shader_aux_3D_udecl_highp;
}

/** @brief Fragment shader tail selector.
 *
 *  @tparam D      Texture dimenisons.
 *
 *  @return Frgment shader source code part.
 */
template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderTail<2>()
{
	return s_fragment_shader_tail_2D;
}

template <>
const glw::GLchar* StorageMultisampleTest::FragmentShaderTail<3>()
{
	return s_fragment_shader_tail_3D;
}

/** @brief Texture target selector.
 *
 *  @tparam D      Texture dimenisons.
 *
 *  @return Texture target.
 */
template <>
glw::GLenum StorageMultisampleTest::InputTextureTarget<2>()
{
	return GL_TEXTURE_2D;
}

template <>
glw::GLenum StorageMultisampleTest::InputTextureTarget<3>()
{
	return GL_TEXTURE_2D_ARRAY;
}

/** @brief Prepare texture data for input texture.
 *
 *  @tparam D      Texture dimenisons.
 *
 *  @note parameters as passed to texImage*
 */
template <>
void StorageMultisampleTest::InputTextureImage<2>(const glw::GLenum internal_format, const glw::GLuint width,
												  const glw::GLuint height, const glw::GLuint depth,
												  const glw::GLenum format, const glw::GLenum type,
												  const glw::GLvoid* data)
{
	(void)depth;
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Data setup. */
	gl.texImage2D(InputTextureTarget<2>(), 0, internal_format, width, height, 0, format, type, data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage2D has failed");
}

/** @brief Prepare texture data for input texture.
 *
 *  @tparam D      Texture dimenisons.
 *
 *  @note parameters as passed to texImage*
 */
template <>
void StorageMultisampleTest::InputTextureImage<3>(const glw::GLenum internal_format, const glw::GLuint width,
												  const glw::GLuint height, const glw::GLuint depth,
												  const glw::GLenum format, const glw::GLenum type,
												  const glw::GLvoid* data)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Data setup. */
	gl.texImage3D(InputTextureTarget<3>(), 0, internal_format, width, height, depth, 0, format, type, data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage2D has failed");
}

/** @brief Create texture.
 *
 *  @tparam T      Type.
 *  @tparam S      Size (# of components).
 *  @tparam N      Is normalized.
 *  @tparam D      Dimmensions.
 */
template <typename T, glw::GLint S, bool N, glw::GLuint D>
void StorageMultisampleTest::CreateInputTexture()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Objects creation. */
	gl.genTextures(1, &m_to);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

	gl.bindTexture(InputTextureTarget<D>(), m_to);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

	/* Data setup. */
	InputTextureImage<D>(InternalFormat<T, S, N>(), TestReferenceDataWidth<D>(), TestReferenceDataHeight<D>(),
						 TestReferenceDataDepth<D>(), Format<S, N>(), Type<T>(), ReferenceData<T, N>());

	/* Parameter setup. */
	gl.texParameteri(InputTextureTarget<D>(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl.texParameteri(InputTextureTarget<D>(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri call failed.");
}

/** @brief Compre results with the reference.
 *
 *  @tparam T      Type.
 *  @tparam S      Size (# of components).
 *  @tparam N      Is normalized.
 *
 *  @return True if equal, false otherwise.
 */
template <typename T, glw::GLint S, bool N, glw::GLuint D>
bool StorageMultisampleTest::Check()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Fetching data fro auxiliary texture. */
	std::vector<T> result(TestReferenceDataCount<S, D>());

	gl.bindTexture(InputTextureTarget<D>() /* Auxiliary target is the same as input. */, m_to_aux);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

	gl.getTexImage(InputTextureTarget<D>() /* Auxiliary target is the same as input. */, 0, Format<S, N>(), Type<T>(),
				   (glw::GLvoid*)(&result[0]));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexImage has failed");

	/* Comparison. */
	for (glw::GLuint i = 0; i < TestReferenceDataCount<S, D>(); ++i)
	{
		if (!Compare<T>(result[i], ReferenceData<T, N>()[i]))
		{
			return false;
		}
	}

	return true;
}

/** @brief Test case function.
 *
 *  @tparam T       Type.
 *  @tparam S       Size (# of components).
 *  @tparam N       Is normalized.
 *  @tparam D       Number of texture dimensions.
 *
 *  @return True if test succeeded, false otherwise.
 */
template <typename T, glw::GLint S, bool N, glw::GLuint D>
bool StorageMultisampleTest::Test()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Setup. */
	gl.pixelStorei(GL_UNPACK_ALIGNMENT, sizeof(T));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPixelStorei has failed");

	gl.pixelStorei(GL_PACK_ALIGNMENT, sizeof(T));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPixelStorei has failed");

	CreateInputTexture<T, S, N, D>();

	if (!PrepareFramebufferMultisample<D>(InternalFormat<T, S, N>()))
	{
		CleanInputTexture();

		return false;
	}

	PrepareFramebufferAuxiliary<D>(InternalFormat<T, S, N>());

	/* Action. */
	Draw<D>();

	/* Compare results with reference. */
	bool result = Check<T, S, N, D>();

	/* Cleanup. */
	CleanAuxiliaryTexture();
	CleanFramebuffers();
	CleanInputTexture();
	CleanErrors();

	/* Pass result. */
	return result;
}

/** @brief Lopp test function over S.
 *
 *  @tparam T      Type.
 *  @tparam N      Is normalized.
 *  @tparam D      Texture dimension.
 *
 *  @param [in] skip_rgb            Skip test of S = 3 (needed for some formats).
 *
 *  @return True if tests succeeded, false otherwise.
 */
template <typename T, bool N, glw::GLuint D>
bool StorageMultisampleTest::LoopTestOverS(bool skip_rgb)
{
	/* Prepare one program to create multisample texture and one to copy it to regular texture. */
	m_po_ms  = PrepareProgram(FragmentShaderDeclarationMultisample<T, N, D>(), FragmentShaderTail<D>());
	m_po_aux = PrepareProgram(FragmentShaderDeclarationAuxiliary<T, N, D>(), FragmentShaderTail<D>());

	/* Run tests. */
	bool result = true;

	result &= Test<T, 1, N, D>();

	result &= Test<T, 2, N, D>();

	if (!skip_rgb)
	{
		result &= Test<T, 3, N, D>();
	}

	result &= Test<T, 4, N, D>();

	/* Cleanup.*/
	CleanPrograms();
	CleanErrors();

	/* Pass result. */
	return result;
}

/** @brief Lopp test function over D and next over S.
 *
 *  @tparam T      Type.
 *  @tparam N      Is normalized.
 *
 *  @param [in] skip_rgb            Skip test of S = 3 (needed for some formats).
 *
 *  @return True if tests succeeded, false otherwise.
 */
template <typename T, bool N>
bool StorageMultisampleTest::LoopTestOverDOverS(bool skip_rgb)
{
	bool result = true;

	result &= LoopTestOverS<T, N, 2>(skip_rgb);
	result &= LoopTestOverS<T, N, 3>(skip_rgb);

	return result;
}

/** @brief Function prepares framebuffer with internal format color attachment.
 *         Viewport is set up. Content of the framebuffer is cleared.
 *
 *  @note The function may throw if unexpected error has occured.
 */
template <>
bool StorageMultisampleTest::PrepareFramebufferMultisample<2>(const glw::GLenum internal_format)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Prepare framebuffer. */
	gl.genFramebuffers(1, &m_fbo_ms);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers call failed.");

	gl.genTextures(1, &m_to_ms);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_ms);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_to_ms);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer call failed.");

	gl.textureStorage2DMultisample(m_to_ms, 1, internal_format, TestReferenceDataWidth<2>(),
								   TestReferenceDataHeight<2>(), false);

	glw::GLenum error;

	if (GL_NO_ERROR != (error = gl.getError()))
	{
		CleanFramebuffers();

		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "glTextureStorage2DMultisample unexpectedly generated error "
			<< glu::getErrorStr(error) << " during the test of internal format "
			<< glu::getTextureFormatStr(internal_format) << ". Test fails." << tcu::TestLog::EndMessage;

		return false;
	}

	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_to_ms, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer call failed.");

	if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		throw 0;
	}

	gl.viewport(0, 0, TestReferenceDataWidth<2>(), TestReferenceDataHeight<2>());
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport call failed.");

	/* Clear framebuffer's content. */
	gl.clearColor(0.0f, 0.0f, 0.0f, 0.0f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor call failed.");

	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear call failed.");

	return true;
}

/** @brief Function prepares framebuffer with internal format color attachment.
 *         Viewport is set up. Content of the framebuffer is cleared.
 *
 *  @note The function may throw if unexpected error has occured.
 */
template <>
bool StorageMultisampleTest::PrepareFramebufferMultisample<3>(const glw::GLenum internal_format)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Prepare framebuffer. */
	gl.genFramebuffers(1, &m_fbo_ms);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers call failed.");

	gl.genTextures(1, &m_to_ms);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_ms);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, m_to_ms);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer call failed.");

	gl.textureStorage3DMultisample(m_to_ms, 1, internal_format, TestReferenceDataWidth<3>(),
								   TestReferenceDataHeight<3>(), TestReferenceDataDepth<3>(), false);

	glw::GLenum error;

	if (GL_NO_ERROR != (error = gl.getError()))
	{
		CleanFramebuffers();

		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "glTextureStorage3DMultisample unexpectedly generated error "
			<< glu::getErrorStr(error) << " during the test of internal format "
			<< glu::getTextureFormatStr(internal_format) << ". Test fails." << tcu::TestLog::EndMessage;

		return false;
	}

	for (glw::GLuint i = 0; i < TestReferenceDataDepth<3>(); ++i)
	{
		gl.framebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, m_to_ms, 0, i);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer call failed.");
	}

	if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		throw 0;
	}

	gl.viewport(0, 0, TestReferenceDataWidth<3>(), TestReferenceDataHeight<3>());
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport call failed.");

	/* Clear framebuffer's content. */
	gl.clearColor(0.0f, 0.0f, 0.0f, 0.0f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor call failed.");

	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear call failed.");

	return true;
}

/** @brief Function prepares framebuffer with internal format color attachment.
 *         Viewport is set up. Content of the framebuffer is cleared.
 *
 *  @note The function may throw if unexpected error has occured.
 */
template <>
void StorageMultisampleTest::PrepareFramebufferAuxiliary<2>(const glw::GLenum internal_format)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Prepare framebuffer. */
	gl.genFramebuffers(1, &m_fbo_aux);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers call failed.");

	gl.genTextures(1, &m_to_aux);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_aux);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	gl.bindTexture(GL_TEXTURE_2D, m_to_aux);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer call failed.");

	gl.textureStorage2D(m_to_aux, 1, internal_format, TestReferenceDataWidth<2>(), TestReferenceDataHeight<2>());
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTextureStorage2D call failed.");

	/* Parameter setup. */
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri call failed.");

	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_to_aux, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer call failed.");

	if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		throw 0;
	}

	gl.viewport(0, 0, TestReferenceDataWidth<2>(), TestReferenceDataHeight<2>());
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport call failed.");

	/* Clear framebuffer's content. */
	gl.clearColor(0.0f, 0.0f, 0.0f, 0.0f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor call failed.");

	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear call failed.");
}

/** @brief Function prepares framebuffer with internal format color attachment.
 *         Viewport is set up. Content of the framebuffer is cleared.
 *
 *  @note The function may throw if unexpected error has occured.
 */
template <>
void StorageMultisampleTest::PrepareFramebufferAuxiliary<3>(const glw::GLenum internal_format)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Prepare framebuffer. */
	gl.genFramebuffers(1, &m_fbo_aux);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers call failed.");

	gl.genTextures(1, &m_to_aux);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_aux);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	gl.bindTexture(GL_TEXTURE_2D_ARRAY, m_to_aux);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer call failed.");

	gl.textureStorage3D(m_to_aux, 1, internal_format, TestReferenceDataWidth<3>(), TestReferenceDataHeight<3>(),
						TestReferenceDataDepth<3>());
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTextureStorage3D call failed.");

	/* Parameter setup. */
	gl.texParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri call failed.");

	for (glw::GLuint i = 0; i < TestReferenceDataDepth<3>(); ++i)
	{
		gl.framebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, m_to_aux, 0, i);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer call failed.");
	}

	if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		throw 0;
	}

	gl.viewport(0, 0, TestReferenceDataWidth<3>(), TestReferenceDataHeight<3>());
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport call failed.");

	/* Clear framebuffer's content. */
	gl.clearColor(0.0f, 0.0f, 0.0f, 0.0f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor call failed.");

	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear call failed.");
}

/** @brief Prepare program
 *
 *  @param [in] variable_declaration      Variables declaration part of fragment shader source code.
 *  @param [in] tail                      Tail part of fragment shader source code.
 */
glw::GLuint StorageMultisampleTest::PrepareProgram(const glw::GLchar* variable_declaration, const glw::GLchar* tail)
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	struct Shader
	{
		glw::GLchar const* source[3];
		glw::GLsizei const count;
		glw::GLenum const  type;
		glw::GLuint		   id;
	} shader[] = { { { s_vertex_shader, NULL, NULL }, 1, GL_VERTEX_SHADER, 0 },
				   { { s_fragment_shader_head, variable_declaration, tail }, 3, GL_FRAGMENT_SHADER, 0 } };

	glw::GLuint const shader_count = sizeof(shader) / sizeof(shader[0]);

	glw::GLuint po = 0;

	try
	{
		/* Create program. */
		po = gl.createProgram();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram call failed.");

		/* Shader compilation. */

		for (glw::GLuint i = 0; i < shader_count; ++i)
		{
			{
				shader[i].id = gl.createShader(shader[i].type);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader call failed.");

				gl.attachShader(po, shader[i].id);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader call failed.");

				gl.shaderSource(shader[i].id, shader[i].count, shader[i].source, NULL);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource call failed.");

				gl.compileShader(shader[i].id);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader call failed.");

				glw::GLint status = GL_FALSE;

				gl.getShaderiv(shader[i].id, GL_COMPILE_STATUS, &status);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv call failed.");

				if (GL_FALSE == status)
				{
					glw::GLint log_size = 0;
					gl.getShaderiv(shader[i].id, GL_INFO_LOG_LENGTH, &log_size);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv call failed.");

					glw::GLchar* log_text = new glw::GLchar[log_size];

					gl.getShaderInfoLog(shader[i].id, log_size, NULL, &log_text[0]);

					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Shader compilation has failed.\n"
						<< "Shader type: " << glu::getShaderTypeStr(shader[i].type) << "\n"
						<< "Shader compilation error log:\n"
						<< log_text << "\n"
						<< "Shader source code:\n"
						<< shader[i].source[0] << (shader[i].source[1] ? shader[i].source[1] : "")
						<< (shader[i].source[2] ? shader[i].source[2] : "") << "\n"
						<< tcu::TestLog::EndMessage;

					delete[] log_text;

					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderInfoLog call failed.");

					throw 0;
				}
			}
		}

		/* Link. */
		gl.linkProgram(po);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings call failed.");

		glw::GLint status = GL_FALSE;

		gl.getProgramiv(po, GL_LINK_STATUS, &status);

		if (GL_TRUE == status)
		{
			for (glw::GLuint i = 0; i < shader_count; ++i)
			{
				if (shader[i].id)
				{
					gl.detachShader(po, shader[i].id);

					GLU_EXPECT_NO_ERROR(gl.getError(), "glDetachShader call failed.");
				}
			}
		}
		else
		{
			glw::GLint log_size = 0;

			gl.getProgramiv(po, GL_INFO_LOG_LENGTH, &log_size);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv call failed.");

			glw::GLchar* log_text = new glw::GLchar[log_size];

			gl.getProgramInfoLog(po, log_size, NULL, &log_text[0]);

			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Program linkage has failed due to:\n"
												<< log_text << "\n"
												<< tcu::TestLog::EndMessage;

			delete[] log_text;

			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramInfoLog call failed.");

			throw 0;
		}
	}
	catch (...)
	{
		if (po)
		{
			gl.deleteProgram(po);

			po = 0;
		}
	}

	for (glw::GLuint i = 0; i < shader_count; ++i)
	{
		if (0 != shader[i].id)
		{
			gl.deleteShader(shader[i].id);

			shader[i].id = 0;
		}
	}

	if (0 == po)
	{
		throw 0;
	}

	return po;
}

/** @brief Prepare VAO.
 */
void StorageMultisampleTest::PrepareVertexArray()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genVertexArrays(1, &m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays has failed");

	gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray has failed");
}

/** @brief Draw call 2D.
 */
template <>
void StorageMultisampleTest::Draw<2>()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Prepare multisample texture using draw call. */

	/* Prepare framebuffer with multisample texture. */
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_ms);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

	/* Use first program program. */
	gl.useProgram(m_po_ms);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram has failed");

	/* Prepare texture to be drawn with. */
	gl.activeTexture(GL_TEXTURE0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture has failed");

	gl.bindTexture(GL_TEXTURE_2D, m_to);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture has failed");

	gl.uniform1i(gl.getUniformLocation(m_po_ms, "texture_input"), 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i or glGetUniformLocation has failed");

	/* Select layer. */
	gl.drawBuffer(GL_COLOR_ATTACHMENT0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawBuffer has failed");

	/* Draw. */
	gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays has failed");

	/* Copy multisample texture to auxiliary texture using draw call. */

	/* Prepare framebuffer with auxiliary texture. */
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_aux);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

	/* Use first program program. */
	gl.useProgram(m_po_aux);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram has failed");

	/* Prepare texture to be drawn with. */
	gl.activeTexture(GL_TEXTURE0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture has failed");

	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_to_ms);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture has failed");

	gl.bindTextureUnit(0, m_to);

	gl.uniform1i(gl.getUniformLocation(m_po_aux, "texture_input"), 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i or glGetUniformLocation has failed");

	/* Select layer. */
	gl.drawBuffer(GL_COLOR_ATTACHMENT0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawBuffer has failed");

	/* Draw. */
	gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays has failed");
}

/** @brief Draw call 3D.
 */
template <>
void StorageMultisampleTest::Draw<3>()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Prepare multisample texture using draw call. */

	/* Prepare framebuffer with multisample texture. */
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_ms);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

	/* Use first program program. */
	gl.useProgram(m_po_ms);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram has failed");

	/* Prepare texture to be drawn with. */
	gl.activeTexture(GL_TEXTURE0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture has failed");

	gl.bindTexture(GL_TEXTURE_2D_ARRAY, m_to);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture has failed");

	gl.uniform1i(gl.getUniformLocation(m_po_ms, "texture_input"), 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i or glGetUniformLocation has failed");

	/* For each texture layer. */
	for (glw::GLuint i = 0; i < TestReferenceDataDepth<3>(); ++i)
	{
		/* Select layer. */
		gl.drawBuffer(GL_COLOR_ATTACHMENT0 + i);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawBuffer has failed");

		gl.uniform1i(gl.getUniformLocation(m_po_aux, "texture_layer"), i);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i or glGetUniformLocation has failed");

		/* Draw. */
		gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays has failed");
	}

	/* Copy multisample texture to auxiliary texture using draw call. */

	/* Prepare framebuffer with auxiliary texture. */
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_aux);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

	/* Use first program program. */
	gl.useProgram(m_po_aux);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram has failed");

	/* Prepare texture to be drawn with. */
	gl.activeTexture(GL_TEXTURE0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture has failed");

	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, m_to_ms);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture has failed");

	gl.bindTextureUnit(0, m_to);

	gl.uniform1i(gl.getUniformLocation(m_po_aux, "texture_input"), 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i or glGetUniformLocation has failed");

	/* For each texture layer. */
	for (glw::GLuint i = 0; i < TestReferenceDataDepth<3>(); ++i)
	{
		/* Select layer. */
		gl.drawBuffer(GL_COLOR_ATTACHMENT0 + i);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawBuffer has failed");

		gl.uniform1i(gl.getUniformLocation(m_po_aux, "texture_layer"), i);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i or glGetUniformLocation has failed");

		/* Draw. */
		gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays has failed");
	}
}

/** @brief Clean GL objects, test variables and GL errors.
 */
void StorageMultisampleTest::CleanInputTexture()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Texture. */
	if (m_to)
	{
		gl.deleteTextures(1, &m_to);

		m_to = 0;
	}
}

/** @brief Clean GL objects, test variables and GL errors.
 */
void StorageMultisampleTest::CleanAuxiliaryTexture()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_to_aux)
	{
		gl.deleteTextures(1, &m_to_aux);

		m_to_aux = 0;
	}
}

/** @brief Clean GL objects, test variables and GL errors.
 */
void StorageMultisampleTest::CleanFramebuffers()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Mulitsample framebuffer. */
	if (m_fbo_ms)
	{
		gl.deleteFramebuffers(1, &m_fbo_ms);

		m_fbo_ms = 0;
	}

	/* Mulitsample texture. */
	if (m_to_ms)
	{
		gl.deleteTextures(1, &m_to_ms);

		m_to_ms = 0;
	}

	/* Auxiliary framebuffer. */
	if (m_fbo_aux)
	{
		gl.deleteFramebuffers(1, &m_fbo_aux);

		m_fbo_aux = 0;
	}

	/* Auxiliary texture. */
	if (m_to_aux)
	{
		gl.deleteTextures(1, &m_to_aux);

		m_to_aux = 0;
	}
}

/** @brief Clean GL objects, test variables and GL errors.
 */
void StorageMultisampleTest::CleanPrograms()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Binding point. */
	gl.useProgram(0);

	/* Multisample texture preparation program. */
	if (m_po_ms)
	{
		gl.deleteProgram(m_po_ms);

		m_po_ms = 0;
	}

	/* Auxiliary texture preparation program. */
	if (m_po_aux)
	{
		gl.deleteProgram(m_po_aux);

		m_po_aux = 0;
	}
}

/** @brief Clean GL objects, test variables and GL errors.
 */
void StorageMultisampleTest::CleanErrors()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Query all errors until GL_NO_ERROR occure. */
	while (GL_NO_ERROR != gl.getError())
		;
}

/** @brief Clean GL objects, test variables and GL errors.
 */
void StorageMultisampleTest::CleanVertexArray()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_vao)
	{
		gl.bindVertexArray(0);

		gl.deleteVertexArrays(1, &m_vao);

		m_vao = 0;
	}
}

/** @brief Iterate Storage Multisample Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult StorageMultisampleTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		PrepareVertexArray();

		//  gl.enable(GL_MULTISAMPLE);

		is_ok &= LoopTestOverDOverS<glw::GLbyte, false>(true);
		is_ok &= LoopTestOverDOverS<glw::GLubyte, false>(true);
		is_ok &= LoopTestOverDOverS<glw::GLshort, false>(true);
		is_ok &= LoopTestOverDOverS<glw::GLushort, false>(true);
		is_ok &= LoopTestOverDOverS<glw::GLint, false>(false);
		is_ok &= LoopTestOverDOverS<glw::GLuint, false>(false);
		is_ok &= LoopTestOverDOverS<glw::GLubyte, true>(true);
		is_ok &= LoopTestOverDOverS<glw::GLushort, true>(true);
		is_ok &= LoopTestOverDOverS<glw::GLfloat, true>(false);
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	CleanInputTexture();
	CleanAuxiliaryTexture();
	CleanFramebuffers();
	CleanPrograms();
	CleanErrors();
	CleanVertexArray();
	gl.disable(GL_MULTISAMPLE);

	/* Errors clean up. */
	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/* Vertex shader source code. */
const glw::GLchar* StorageMultisampleTest::s_vertex_shader = "#version 450\n"
															 "\n"
															 "void main()\n"
															 "{\n"
															 "    switch(gl_VertexID)\n"
															 "    {\n"
															 "        case 0:\n"
															 "            gl_Position = vec4(-1.0, 1.0, 0.0, 1.0);\n"
															 "            break;\n"
															 "        case 1:\n"
															 "            gl_Position = vec4( 1.0, 1.0, 0.0, 1.0);\n"
															 "            break;\n"
															 "        case 2:\n"
															 "            gl_Position = vec4(-1.0,-1.0, 0.0, 1.0);\n"
															 "            break;\n"
															 "        case 3:\n"
															 "            gl_Position = vec4( 1.0,-1.0, 0.0, 1.0);\n"
															 "            break;\n"
															 "    }\n"
															 "}\n";

/* Fragment shader source program. */
const glw::GLchar* StorageMultisampleTest::s_fragment_shader_head =
	"#version 450\n"
	"\n"
	"layout(pixel_center_integer) in vec4 gl_FragCoord;\n"
	"\n";

const glw::GLchar* StorageMultisampleTest::s_fragment_shader_ms_2D_fdecl_lowp =
	"uniform  sampler2D texture_input;\nout     vec4          texture_output;\n";
const glw::GLchar* StorageMultisampleTest::s_fragment_shader_ms_2D_idecl_lowp =
	"uniform isampler2D texture_input;\nout     ivec4         texture_output;\n";
const glw::GLchar* StorageMultisampleTest::s_fragment_shader_ms_2D_udecl_lowp =
	"uniform usampler2D texture_input;\nout     uvec4         texture_output;\n";
const glw::GLchar* StorageMultisampleTest::s_fragment_shader_ms_2D_fdecl_mediump =
	"uniform  sampler2D texture_input;\nout     vec4          texture_output;\n";
const glw::GLchar* StorageMultisampleTest::s_fragment_shader_ms_2D_idecl_mediump =
	"uniform isampler2D texture_input;\nout     ivec4         texture_output;\n";
const glw::GLchar* StorageMultisampleTest::s_fragment_shader_ms_2D_udecl_mediump =
	"uniform usampler2D texture_input;\nout     uvec4         texture_output;\n";
const glw::GLchar* StorageMultisampleTest::s_fragment_shader_ms_2D_fdecl_highp =
	"uniform  sampler2D texture_input;\nout     vec4          texture_output;\n";
const glw::GLchar* StorageMultisampleTest::s_fragment_shader_ms_2D_idecl_highp =
	"uniform isampler2D texture_input;\nout     ivec4         texture_output;\n";
const glw::GLchar* StorageMultisampleTest::s_fragment_shader_ms_2D_udecl_highp =
	"uniform usampler2D texture_input;\nout     uvec4         texture_output;\n";

const glw::GLchar* StorageMultisampleTest::s_fragment_shader_ms_3D_fdecl_lowp =
	"uniform  sampler2DArray texture_input;\nout     vec4          texture_output;\n";

const glw::GLchar* StorageMultisampleTest::s_fragment_shader_ms_3D_idecl_lowp =
	"uniform isampler2DArray texture_input;\nout     ivec4         texture_output;\n";

const glw::GLchar* StorageMultisampleTest::s_fragment_shader_ms_3D_udecl_lowp =
	"uniform usampler2DArray texture_input;\nout     uvec4         texture_output;\n";

const glw::GLchar* StorageMultisampleTest::s_fragment_shader_ms_3D_fdecl_mediump =
	"uniform  sampler2DArray texture_input;\nout     vec4          texture_output;\n";

const glw::GLchar* StorageMultisampleTest::s_fragment_shader_ms_3D_idecl_mediump =
	"uniform isampler2DArray texture_input;\nout     ivec4         texture_output;\n";

const glw::GLchar* StorageMultisampleTest::s_fragment_shader_ms_3D_udecl_mediump =
	"uniform usampler2DArray texture_input;\nout     uvec4         texture_output;\n";

const glw::GLchar* StorageMultisampleTest::s_fragment_shader_ms_3D_fdecl_highp =
	"uniform  sampler2DArray texture_input;\nout     vec4          texture_output;\n";

const glw::GLchar* StorageMultisampleTest::s_fragment_shader_ms_3D_idecl_highp =
	"uniform isampler2DArray texture_input;\nout     ivec4         texture_output;\n";

const glw::GLchar* StorageMultisampleTest::s_fragment_shader_ms_3D_udecl_highp =
	"uniform usampler2DArray texture_input;\nout     uvec4         texture_output;\n";

const glw::GLchar* StorageMultisampleTest::s_fragment_shader_aux_2D_fdecl_lowp =
	"uniform  sampler2DMS texture_input;\nout     vec4          texture_output;\n";
const glw::GLchar* StorageMultisampleTest::s_fragment_shader_aux_2D_idecl_lowp =
	"uniform isampler2DMS texture_input;\nout     ivec4         texture_output;\n";
const glw::GLchar* StorageMultisampleTest::s_fragment_shader_aux_2D_udecl_lowp =
	"uniform usampler2DMS texture_input;\nout     uvec4         texture_output;\n";

const glw::GLchar* StorageMultisampleTest::s_fragment_shader_aux_2D_fdecl_mediump =
	"uniform  sampler2DMS texture_input;\nout     vec4          texture_output;\n";

const glw::GLchar* StorageMultisampleTest::s_fragment_shader_aux_2D_idecl_mediump =
	"uniform isampler2DMS texture_input;\nout     ivec4         texture_output;\n";

const glw::GLchar* StorageMultisampleTest::s_fragment_shader_aux_2D_udecl_mediump =
	"uniform usampler2DMS texture_input;\nout     uvec4         texture_output;\n";

const glw::GLchar* StorageMultisampleTest::s_fragment_shader_aux_2D_fdecl_highp =
	"uniform  sampler2DMS texture_input;\nout     vec4          texture_output;\n";
const glw::GLchar* StorageMultisampleTest::s_fragment_shader_aux_2D_idecl_highp =
	"uniform isampler2DMS texture_input;\nout     ivec4         texture_output;\n";
const glw::GLchar* StorageMultisampleTest::s_fragment_shader_aux_2D_udecl_highp =
	"uniform usampler2DMS texture_input;\nout     uvec4         texture_output;\n";

const glw::GLchar* StorageMultisampleTest::s_fragment_shader_aux_3D_fdecl_lowp =
	"uniform  sampler2DMSArray texture_input;\nout     vec4          texture_output;\n";

const glw::GLchar* StorageMultisampleTest::s_fragment_shader_aux_3D_idecl_lowp =
	"uniform isampler2DMSArray texture_input;\nout     ivec4         texture_output;\n";

const glw::GLchar* StorageMultisampleTest::s_fragment_shader_aux_3D_udecl_lowp =
	"uniform usampler2DMSArray texture_input;\nout     uvec4         texture_output;\n";

const glw::GLchar* StorageMultisampleTest::s_fragment_shader_aux_3D_fdecl_mediump =
	"uniform  sampler2DMSArray texture_input;\nout     vec4          texture_output;\n";

const glw::GLchar* StorageMultisampleTest::s_fragment_shader_aux_3D_idecl_mediump =
	"uniform isampler2DMSArray texture_input;\nout     ivec4         texture_output;\n";

const glw::GLchar* StorageMultisampleTest::s_fragment_shader_aux_3D_udecl_mediump =
	"uniform usampler2DMSArray texture_input;\nout     uvec4         texture_output;\n";

const glw::GLchar* StorageMultisampleTest::s_fragment_shader_aux_3D_fdecl_highp =
	"uniform  sampler2DMSArray texture_input;\nout     vec4          texture_output;\n";

const glw::GLchar* StorageMultisampleTest::s_fragment_shader_aux_3D_idecl_highp =
	"uniform isampler2DMSArray texture_input;\nout     ivec4         texture_output;\n";

const glw::GLchar* StorageMultisampleTest::s_fragment_shader_aux_3D_udecl_highp =
	"uniform usampler2DMSArray texture_input;\nout     uvec4         texture_output;\n";

const glw::GLchar* StorageMultisampleTest::s_fragment_shader_tail_2D =
	"\n"
	"void main()\n"
	"{\n"
	"    texture_output = texelFetch(texture_input, ivec2(gl_FragCoord.xy), 0);\n"
	"}\n";

const glw::GLchar* StorageMultisampleTest::s_fragment_shader_tail_3D =
	"\n"
	"uniform int texture_layer;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    texture_output = texelFetch(texture_input, ivec3(gl_FragCoord.xy, texture_layer), 0);\n"
	"}\n";

/******************************** Compressed SubImage Test Implementation   ********************************/

/** @brief Compressed SubImage Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
CompressedSubImageTest::CompressedSubImageTest(deqp::Context& context)
	: deqp::TestCase(context, "textures_compressed_subimage", "Texture Compressed SubImage Test")
	, m_to(0)
	, m_to_aux(0)
	, m_reference(DE_NULL)
	, m_result(DE_NULL)
	, m_reference_size(0)
	, m_reference_internalformat(0)
{
	/* Intentionally left blank. */
}

/** @brief Create texture.
 *
 *  @param [in] target      Texture target.
 */
void CompressedSubImageTest::CreateTextures(glw::GLenum target)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Auxiliary texture (for content creation). */
	gl.genTextures(1, &m_to_aux);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

	gl.bindTexture(target, m_to_aux);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

	/* Test texture (for data upload). */
	gl.genTextures(1, &m_to);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

	gl.bindTexture(target, m_to);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");
}

/** @brief Texture target selector.
 *
 *  @tparam D      Texture dimenisons.
 *
 *  @return Texture target.
 */
template <>
glw::GLenum CompressedSubImageTest::TextureTarget<1>()
{
	return GL_TEXTURE_1D;
}

template <>
glw::GLenum CompressedSubImageTest::TextureTarget<2>()
{
	return GL_TEXTURE_2D;
}

template <>
glw::GLenum CompressedSubImageTest::TextureTarget<3>()
{
	return GL_TEXTURE_2D_ARRAY;
}

/** @brief Prepare texture data for the auxiliary texture.
 *
 *  @tparam D      Texture dimenisons.
 *
 *  @note parameters as passed to texImage*
 */
template <>
void CompressedSubImageTest::TextureImage<1>(glw::GLint internalformat)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.texImage1D(TextureTarget<1>(), 0, internalformat, s_texture_width, 0, GL_RGBA, GL_UNSIGNED_BYTE, s_texture_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage1D has failed");
}

/** @brief Prepare texture data for the auxiliary texture.
 *
 *  @tparam D      Texture dimenisons.
 *
 *  @note parameters as passed to texImage*
 */
template <>
void CompressedSubImageTest::TextureImage<2>(glw::GLint internalformat)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.texImage2D(TextureTarget<2>(), 0, internalformat, s_texture_width, s_texture_height, 0, GL_RGBA,
				  GL_UNSIGNED_BYTE, s_texture_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage2D has failed");
}

/** @brief Prepare texture data for the auxiliary texture.
 *
 *  @tparam D      Texture dimenisons.
 *
 *  @note parameters as passed to texImage*
 */
template <>
void CompressedSubImageTest::TextureImage<3>(glw::GLint internalformat)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.texImage3D(TextureTarget<3>(), 0, internalformat, s_texture_width, s_texture_height, s_texture_depth, 0, GL_RGBA,
				  GL_UNSIGNED_BYTE, s_texture_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage3D has failed");
}

/** @brief Prepare texture data for the compressed texture.
 *
 *  @tparam D      Texture dimenisons.
 *
 *  @param [in] internalformat      Texture internal format.
 *
 *  @return True if tested function succeeded, false otherwise.
 */
template <>
bool CompressedSubImageTest::CompressedTextureSubImage<1>(glw::GLint internalformat)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Load texture image with tested function. */
	if (m_reference_size)
	{
		gl.compressedTextureSubImage1D(m_to, 0, 0, s_texture_width, internalformat, m_reference_size, m_reference);
	}
	else
	{
		/* For 1D version there is no specific compressed texture internal format spcified in OpenGL 4.5 core profile documentation.
		 Only implementation depended specific internalformats may provide this functionality. As a result there may be no reference data to be substituted.
		 Due to this reason there is no use of CompressedTextureSubImage1D and particulary it cannot be tested. */
		return true;
	}

	/* Check errors. */
	glw::GLenum error;

	if (GL_NO_ERROR != (error = gl.getError()))
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "glCompressedTextureSubImage1D unexpectedly generated error "
			<< glu::getErrorStr(error) << " during the test with internal format "
			<< glu::getTextureFormatStr(internalformat) << ". Test fails." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Prepare texture data for the compressed texture.
 *
 *  @tparam D      Texture dimenisons.
 *
 *  @param [in] internalformat      Texture internal format.
 *
 *  @return True if tested function succeeded, false otherwise.
 */
template <>
bool CompressedSubImageTest::CompressedTextureSubImage<2>(glw::GLint internalformat)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Load texture image with tested function. */
	gl.compressedTextureSubImage2D(m_to, 0, 0, 0, s_texture_width, s_texture_height, internalformat, m_reference_size,
								   m_reference);

	/* Check errors. */
	glw::GLenum error;

	if (GL_NO_ERROR != (error = gl.getError()))
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "glCompressedTextureSubImage2D unexpectedly generated error "
			<< glu::getErrorStr(error) << " during the test with internal format "
			<< glu::getTextureFormatStr(internalformat) << ". Test fails." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Prepare texture data for the compressed texture.
 *
 *  @tparam D      Texture dimenisons.
 *
 *  @param [in] internalformat      Texture internal format.
 *
 *  @return True if tested function succeeded, false otherwise.
 */
template <>
bool CompressedSubImageTest::CompressedTextureSubImage<3>(glw::GLint internalformat)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Load texture image with tested function. */
	gl.compressedTextureSubImage3D(m_to, 0, 0, 0, 0, s_texture_width, s_texture_height, s_texture_depth, internalformat,
								   m_reference_size, m_reference);

	/* Check errors. */
	glw::GLenum error;

	if (GL_NO_ERROR != (error = gl.getError()))
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "glCompressedTextureSubImage2D unexpectedly generated error "
			<< glu::getErrorStr(error) << " during the test with internal format "
			<< glu::getTextureFormatStr(internalformat) << ". Test fails." << tcu::TestLog::EndMessage;

		return false;
	}
	return true;
}

/** @brief Prepare the reference data.
 *
 *  @tparam D      Texture dimenisons.
 */
template <glw::GLuint D>
void CompressedSubImageTest::PrepareReferenceData(glw::GLenum internalformat)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Using OpenGL to compress raw data. */
	gl.bindTexture(TextureTarget<D>(), m_to_aux);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

	TextureImage<D>(internalformat);

	/* Sanity checks. */
	if (DE_NULL != m_reference)
	{
		throw 0;
	}

	/* Check that really compressed texture. */
	glw::GLint is_compressed_texture = 0;
	gl.getTexLevelParameteriv(TextureTarget<D>(), 0, GL_TEXTURE_COMPRESSED, &is_compressed_texture);

	if (is_compressed_texture)
	{
		/* Query texture size. */
		glw::GLint compressed_texture_size = 0;
		gl.getTexLevelParameteriv(TextureTarget<D>(), 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &compressed_texture_size);

		/* If compressed then download. */
		if (compressed_texture_size)
		{
			/* Prepare storage. */
			m_reference = new glw::GLubyte[compressed_texture_size];

			if (DE_NULL != m_reference)
			{
				m_reference_size = compressed_texture_size;
			}
			else
			{
				throw 0;
			}

			/* Download compressed texture image. */
			gl.getCompressedTexImage(TextureTarget<D>(), 0, m_reference);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetCompressedTexImage has failed");
		}
	}
}

/** @brief Prepare texture storage.
 *
 *  @tparam D      Texture dimenisons.
 *
 *  @param [in] internalformat      Texture internal format.
 */
template <>
void CompressedSubImageTest::PrepareStorage<1>(glw::GLenum internalformat)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindTexture(TextureTarget<1>(), m_to);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

	gl.texImage1D(TextureTarget<1>(), 0, internalformat, s_texture_width, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage1D has failed");
}

/** @brief Prepare texture storage.
 *
 *  @tparam D      Texture dimenisons.
 *
 *  @param [in] internalformat      Texture internal format.
 */
template <>
void CompressedSubImageTest::PrepareStorage<2>(glw::GLenum internalformat)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindTexture(TextureTarget<2>(), m_to);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

	gl.texImage2D(TextureTarget<2>(), 0, internalformat, s_texture_width, s_texture_height, 0, GL_RGBA,
				  GL_UNSIGNED_BYTE, NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage1D has failed");
}

/** @brief Prepare texture storage.
 *
 *  @tparam D      Texture dimenisons.
 *
 *  @param [in] internalformat      Texture internal format.
 */
template <>
void CompressedSubImageTest::PrepareStorage<3>(glw::GLenum internalformat)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindTexture(TextureTarget<3>(), m_to);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

	gl.texImage3D(TextureTarget<3>(), 0, internalformat, s_texture_width, s_texture_height, s_texture_depth, 0, GL_RGBA,
				  GL_UNSIGNED_BYTE, NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage1D has failed");
}

/** @brief Compre results with the reference.
 *
 *  @tparam T      Type.
 *  @tparam S      Size (# of components).
 *  @tparam N      Is normalized.
 *
 *  @param [in] target              Texture target.
 *  @param [in] internalformat      Texture internal format.
 *
 *  @return True if equal, false otherwise.
 */
bool CompressedSubImageTest::CheckData(glw::GLenum target, glw::GLenum internalformat)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check texture content with reference. */
	m_result = new glw::GLubyte[m_reference_size];

	if (DE_NULL == m_result)
	{
		throw 0;
	}

	gl.getCompressedTexImage(target, 0, m_result);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetCompressedTexImage has failed");

	for (glw::GLuint i = 0; i < m_reference_size; ++i)
	{
		if (m_reference[i] != m_result[i])
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "glCompressedTextureSubImage*D created texture with data "
				<< DataToString(m_reference_size, m_reference) << " however texture contains data "
				<< DataToString(m_reference_size, m_result) << ". Texture target was "
				<< glu::getTextureTargetStr(target) << " and internal format was "
				<< glu::getTextureFormatStr(internalformat) << ". Test fails." << tcu::TestLog::EndMessage;

			return false;
		}
	}

	return true;
}

/** @brief Test case function.
 *
 *  @tparam D       Number of texture dimensions.
 *
 *  @param [in] internal format     Texture internal format.
 *
 *  @return True if test succeeded, false otherwise.
 */
template <glw::GLuint D>
bool CompressedSubImageTest::Test(glw::GLenum internalformat)
{
	/* Create texture image. */
	CreateTextures(TextureTarget<D>());
	PrepareReferenceData<D>(internalformat);
	PrepareStorage<D>(internalformat);

	/* Setup data with CompressedTextureSubImage<D>D function and check for errors. */
	if (!CompressedTextureSubImage<D>(internalformat))
	{
		CleanAll();

		return false;
	}

	/* If compressed reference data was generated than compare values. */
	if (m_reference)
	{
		if (!CheckData(TextureTarget<D>(), internalformat))
		{
			CleanAll();

			return false;
		}
	}

	CleanAll();

	return true;
}

/** @brief Clean GL objects, test variables and GL errors.
 */
void CompressedSubImageTest::CleanAll()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Textures. */
	if (m_to)
	{
		gl.deleteTextures(1, &m_to);

		m_to = 0;
	}

	if (m_to_aux)
	{
		gl.deleteTextures(1, &m_to_aux);

		m_to_aux = 0;
	}

	/* Reference data storage. */
	if (DE_NULL != m_reference)
	{
		delete[] m_reference;

		m_reference = DE_NULL;
	}

	if (DE_NULL != m_result)
	{
		delete[] m_result;

		m_result = DE_NULL;
	}

	m_reference_size = 0;

	/* Errors. */
	while (GL_NO_ERROR != gl.getError())
		;
}

/** @brief Convert raw data into string for logging purposes.
 *
 *  @param [in] count      Count of the data.
 *  @param [in] data       Raw data.
 *
 *  @return String representation of data.
 */
std::string CompressedSubImageTest::DataToString(glw::GLuint count, const glw::GLubyte data[])
{
	std::string data_str = "[";

	for (glw::GLuint i = 0; i < count; ++i)
	{
		std::stringstream int_sstream;

		int_sstream << unsigned(data[i]);

		data_str.append(int_sstream.str());

		if (i + 1 < count)
		{
			data_str.append(", ");
		}
		else
		{
			data_str.append("]");
		}
	}

	return data_str;
}

/** @brief Iterate Compressed SubImage Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult CompressedSubImageTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		is_ok &= Test<1>(GL_COMPRESSED_RGB);

		is_ok &= Test<2>(GL_COMPRESSED_RED_RGTC1);
		is_ok &= Test<2>(GL_COMPRESSED_SIGNED_RED_RGTC1);
		is_ok &= Test<2>(GL_COMPRESSED_RG_RGTC2);
		is_ok &= Test<2>(GL_COMPRESSED_SIGNED_RG_RGTC2);
		is_ok &= Test<2>(GL_COMPRESSED_RGBA_BPTC_UNORM);
		is_ok &= Test<2>(GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM);
		is_ok &= Test<2>(GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT);
		is_ok &= Test<2>(GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT);
		is_ok &= Test<2>(GL_COMPRESSED_RGB8_ETC2);
		is_ok &= Test<2>(GL_COMPRESSED_SRGB8_ETC2);
		is_ok &= Test<2>(GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2);
		is_ok &= Test<2>(GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2);
		is_ok &= Test<2>(GL_COMPRESSED_RGBA8_ETC2_EAC);
		is_ok &= Test<2>(GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC);
		is_ok &= Test<2>(GL_COMPRESSED_R11_EAC);
		is_ok &= Test<2>(GL_COMPRESSED_SIGNED_R11_EAC);
		is_ok &= Test<2>(GL_COMPRESSED_RG11_EAC);
		is_ok &= Test<2>(GL_COMPRESSED_SIGNED_RG11_EAC);

		is_ok &= Test<3>(GL_COMPRESSED_RED_RGTC1);
		is_ok &= Test<3>(GL_COMPRESSED_SIGNED_RED_RGTC1);
		is_ok &= Test<3>(GL_COMPRESSED_RG_RGTC2);
		is_ok &= Test<3>(GL_COMPRESSED_SIGNED_RG_RGTC2);
		is_ok &= Test<3>(GL_COMPRESSED_RGBA_BPTC_UNORM);
		is_ok &= Test<3>(GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM);
		is_ok &= Test<3>(GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT);
		is_ok &= Test<3>(GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT);
		is_ok &= Test<3>(GL_COMPRESSED_RGB8_ETC2);
		is_ok &= Test<3>(GL_COMPRESSED_SRGB8_ETC2);
		is_ok &= Test<3>(GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2);
		is_ok &= Test<3>(GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2);
		is_ok &= Test<3>(GL_COMPRESSED_RGBA8_ETC2_EAC);
		is_ok &= Test<3>(GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC);
		is_ok &= Test<3>(GL_COMPRESSED_R11_EAC);
		is_ok &= Test<3>(GL_COMPRESSED_SIGNED_R11_EAC);
		is_ok &= Test<3>(GL_COMPRESSED_RG11_EAC);
		is_ok &= Test<3>(GL_COMPRESSED_SIGNED_RG11_EAC);
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	CleanAll();

	/* Errors clean up. */
	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** Reference data. */
const glw::GLubyte CompressedSubImageTest::s_texture_data[] = {
	0x00, 0x00, 0x00, 0xFF, 0x7f, 0x7f, 0x7f, 0x00, 0xc3, 0xc3, 0xc3, 0xff, 0xff, 0xff, 0xff, 0x00,
	0x88, 0x00, 0x15, 0xFF, 0xed, 0x1c, 0x24, 0x00, 0xff, 0x7f, 0x27, 0xff, 0xff, 0xf2, 0x00, 0x00,
	0xc8, 0xbf, 0xe7, 0xFF, 0x70, 0x92, 0xbe, 0x00, 0x99, 0xd9, 0xea, 0xff, 0xb5, 0xe6, 0x1d, 0x00,
	0xa3, 0x49, 0xa4, 0xFF, 0x3f, 0x48, 0xcc, 0x00, 0x00, 0xa2, 0xe8, 0xff, 0x22, 0xb1, 0x4c, 0x00,

	0xa3, 0x49, 0xa4, 0xFF, 0xc8, 0xbf, 0xe7, 0x00, 0x88, 0x00, 0x15, 0xff, 0x00, 0x00, 0x00, 0x00,
	0x3f, 0x48, 0xcc, 0xFF, 0x70, 0x92, 0xbe, 0x00, 0xed, 0x1c, 0x24, 0xff, 0x7f, 0x7f, 0x7f, 0x00,
	0x00, 0xa2, 0xe8, 0xFF, 0x99, 0xd9, 0xea, 0x00, 0xff, 0x7f, 0x27, 0xff, 0xc3, 0xc3, 0xc3, 0x00,
	0x22, 0xb1, 0x4c, 0xFF, 0xb5, 0xe6, 0x1d, 0x00, 0xff, 0xf2, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00,

	0x22, 0xb1, 0x4c, 0xFF, 0x00, 0xa2, 0xe8, 0x00, 0x3f, 0x48, 0xcc, 0xff, 0xa3, 0x49, 0xa4, 0x00,
	0xb5, 0xe6, 0x1d, 0xFF, 0x99, 0xd9, 0xea, 0x00, 0x70, 0x92, 0xbe, 0xff, 0xc8, 0xbf, 0xe7, 0x00,
	0xff, 0xf2, 0x00, 0xFF, 0xff, 0x7f, 0x27, 0x00, 0xed, 0x1c, 0x24, 0xff, 0x88, 0x00, 0x15, 0x00,
	0xff, 0xff, 0xff, 0xFF, 0xc3, 0xc3, 0xc3, 0x00, 0x7f, 0x7f, 0x7f, 0xff, 0x00, 0x00, 0x00, 0x00,

	0xff, 0xff, 0xff, 0xFF, 0xff, 0xf2, 0x00, 0x00, 0xb5, 0xe6, 0x1d, 0xff, 0x22, 0xb1, 0x4c, 0x00,
	0xc3, 0xc3, 0xc3, 0xFF, 0xff, 0x7f, 0x27, 0x00, 0x99, 0xd9, 0xea, 0xff, 0x00, 0xa2, 0xe8, 0x00,
	0x7f, 0x7f, 0x7f, 0xFF, 0xed, 0x1c, 0x24, 0x00, 0x70, 0x92, 0xbe, 0xff, 0x3f, 0x48, 0xcc, 0x00,
	0x00, 0x00, 0x00, 0xFF, 0x88, 0x00, 0x15, 0x00, 0xc8, 0xbf, 0xe7, 0xff, 0xa3, 0x49, 0xa4, 0x00
};

/** Reference data parameters. */
const glw::GLuint CompressedSubImageTest::s_texture_width  = 4;
const glw::GLuint CompressedSubImageTest::s_texture_height = 4;
const glw::GLuint CompressedSubImageTest::s_texture_depth  = 4;

/******************************** Copy SubImage Test Implementation   ********************************/

/** @brief Compressed SubImage Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
CopyTest::CopyTest(deqp::Context& context)
	: deqp::TestCase(context, "textures_copy", "Texture Copy Test")
	, m_fbo(0)
	, m_to_src(0)
	, m_to_dst(0)
	, m_result(DE_NULL)
{
	/* Intentionally left blank. */
}

/** @brief Texture target selector.
 *
 *  @tparam D      Texture dimenisons.
 *
 *  @return Texture target.
 */
template <>
glw::GLenum CopyTest::TextureTarget<1>()
{
	return GL_TEXTURE_1D;
}
template <>
glw::GLenum CopyTest::TextureTarget<2>()
{
	return GL_TEXTURE_2D;
}
template <>
glw::GLenum CopyTest::TextureTarget<3>()
{
	return GL_TEXTURE_3D;
}

/** @brief Copy texture, check errors and log.
 *
 *  @note Parameters as passed to CopyTextureSubImage*D
 *
 *  @return True if no error was generated by CopyTextureSubImage*D, false otherwise
 */
bool CopyTest::CopyTextureSubImage1DAndCheckErrors(glw::GLuint texture, glw::GLint level, glw::GLint xoffset,
												   glw::GLint x, glw::GLint y, glw::GLsizei width)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.readBuffer(GL_COLOR_ATTACHMENT0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetCompressedTexImage has failed");

	gl.copyTextureSubImage1D(texture, level, xoffset, x, y, width);

	/* Check errors. */
	glw::GLenum error;

	if (GL_NO_ERROR != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "glCopyTextureSubImage1D unexpectedly generated error "
											<< glu::getErrorStr(error) << ". Test fails." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Copy texture, check errors and log.
 *
 *  @note Parameters as passed to CopyTextureSubImage*D
 *
 *  @return True if no error was generated by CopyTextureSubImage*D, false otherwise
 */
bool CopyTest::CopyTextureSubImage2DAndCheckErrors(glw::GLuint texture, glw::GLint level, glw::GLint xoffset,
												   glw::GLint yoffset, glw::GLint x, glw::GLint y, glw::GLsizei width,
												   glw::GLsizei height)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.readBuffer(GL_COLOR_ATTACHMENT0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetCompressedTexImage has failed");

	gl.copyTextureSubImage2D(texture, level, xoffset, yoffset, x, y, width, height);

	/* Check errors. */
	glw::GLenum error;

	if (GL_NO_ERROR != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "glCopyTextureSubImage2D unexpectedly generated error "
											<< glu::getErrorStr(error) << ". Test fails." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Copy texture, check errors and log.
 *
 *  @note Parameters as passed to CopyTextureSubImage*D
 *
 *  @return True if no error was generated by CopyTextureSubImage*D, false otherwise
 */
bool CopyTest::CopyTextureSubImage3DAndCheckErrors(glw::GLuint texture, glw::GLint level, glw::GLint xoffset,
												   glw::GLint yoffset, glw::GLint zoffset, glw::GLint x, glw::GLint y,
												   glw::GLsizei width, glw::GLsizei height)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.readBuffer(GL_COLOR_ATTACHMENT0 + zoffset);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetCompressedTexImage has failed");

	gl.copyTextureSubImage3D(texture, level, xoffset, yoffset, zoffset, x, y, width, height);

	/* Check errors. */
	glw::GLenum error;

	if (GL_NO_ERROR != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "glCopyTextureSubImage3D unexpectedly generated error "
											<< glu::getErrorStr(error) << ". Test fails." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Create texture.
 *
 *  @tparam D      Dimmensions.
 */
template <>
void CopyTest::CreateSourceTexture<1>()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genTextures(1, &m_to_src);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers call failed.");

	gl.bindTexture(TextureTarget<1>(), m_to_src);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer call failed.");

	gl.texImage1D(TextureTarget<1>(), 0, GL_RGBA8, s_texture_width, 0, GL_RGBA, GL_UNSIGNED_BYTE, s_texture_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage1D call failed.");
}

/** @brief Create texture.
 *
 *  @tparam D      Dimmensions.
 */
template <>
void CopyTest::CreateSourceTexture<2>()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genTextures(1, &m_to_src);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers call failed.");

	gl.bindTexture(TextureTarget<2>(), m_to_src);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer call failed.");

	gl.texImage2D(TextureTarget<2>(), 0, GL_RGBA8, s_texture_width, s_texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
				  s_texture_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage1D call failed.");
}

/** @brief Create texture.
 *
 *  @tparam D      Dimmensions.
 */
template <>
void CopyTest::CreateSourceTexture<3>()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genTextures(1, &m_to_src);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers call failed.");

	gl.bindTexture(TextureTarget<3>(), m_to_src);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer call failed.");

	gl.texImage3D(TextureTarget<3>(), 0, GL_RGBA8, s_texture_width, s_texture_height, s_texture_depth, 0, GL_RGBA,
				  GL_UNSIGNED_BYTE, s_texture_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage1D call failed.");
}

/** @brief Create texture.
 *
 *  @tparam D      Dimmensions.
 */
template <>
void CopyTest::CreateDestinationTexture<1>()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genTextures(1, &m_to_dst);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers call failed.");

	gl.bindTexture(TextureTarget<1>(), m_to_dst);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer call failed.");

	gl.texImage1D(TextureTarget<1>(), 0, GL_RGBA8, s_texture_width, 0, GL_RGBA, GL_UNSIGNED_BYTE, DE_NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage1D call failed.");
}

/** @brief Create texture.
 *
 *  @tparam D      Dimmensions.
 */
template <>
void CopyTest::CreateDestinationTexture<2>()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genTextures(1, &m_to_dst);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers call failed.");

	gl.bindTexture(TextureTarget<2>(), m_to_dst);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer call failed.");

	gl.texImage2D(TextureTarget<2>(), 0, GL_RGBA8, s_texture_width, s_texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
				  DE_NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage1D call failed.");
}

/** @brief Create texture.
 *
 *  @tparam D      Dimmensions.
 */
template <>
void CopyTest::CreateDestinationTexture<3>()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genTextures(1, &m_to_dst);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers call failed.");

	gl.bindTexture(TextureTarget<3>(), m_to_dst);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer call failed.");

	gl.texImage3D(TextureTarget<3>(), 0, GL_RGBA8, s_texture_width, s_texture_height, s_texture_depth, 0, GL_RGBA,
				  GL_UNSIGNED_BYTE, DE_NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage1D call failed.");
}

/** @brief Create framebuffer.
 *
 *  @tparam D      Dimmensions.
 */
template <>
void CopyTest::CreateSourceFramebuffer<1>()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Prepare framebuffer. */
	gl.genFramebuffers(1, &m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	gl.framebufferTexture1D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, TextureTarget<1>(), m_to_src, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture1D call failed.");

	if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		throw 0;
	}

	gl.viewport(0, 0, s_texture_width, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport call failed.");
}

/** @brief Create framebuffer.
 *
 *  @tparam D      Dimmensions.
 */
template <>
void CopyTest::CreateSourceFramebuffer<2>()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Prepare framebuffer. */
	gl.genFramebuffers(1, &m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, TextureTarget<2>(), m_to_src, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture1D call failed.");

	if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		throw 0;
	}

	gl.viewport(0, 0, s_texture_width, s_texture_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport call failed.");
}

/** @brief Create framebuffer.
 *
 *  @tparam D      Dimmensions.
 */
template <>
void CopyTest::CreateSourceFramebuffer<3>()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Prepare framebuffer. */
	gl.genFramebuffers(1, &m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	for (glw::GLuint i = 0; i < s_texture_depth; ++i)
	{
		gl.framebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, TextureTarget<3>(), m_to_src, 0, i);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture1D call failed.");
	}

	if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		throw 0;
	}

	gl.viewport(0, 0, s_texture_width, s_texture_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport call failed.");
}

/** @brief Dispatch function to create test objects */
template <glw::GLuint D>
void				  CopyTest::CreateAll()
{
	CreateSourceTexture<D>();
	CreateSourceFramebuffer<D>();
	CreateDestinationTexture<D>();
}

/** @brief Test function */
template <>
bool CopyTest::Test<1>()
{
	CreateAll<1>();

	bool result = true;

	result &= CopyTextureSubImage1DAndCheckErrors(m_to_dst, 0, 0, 0, 0, s_texture_width / 2);
	result &= CopyTextureSubImage1DAndCheckErrors(m_to_dst, 0, s_texture_width / 2, s_texture_width / 2, 0,
												  s_texture_width / 2);

	result &= CheckData(TextureTarget<1>(), 4 /* RGBA */ * s_texture_width);

	CleanAll();

	return result;
}

/** @brief Test function */
template <>
bool CopyTest::Test<2>()
{
	CreateAll<2>();

	bool result = true;

	result &= CopyTextureSubImage2DAndCheckErrors(m_to_dst, 0, 0, 0, 0, 0, s_texture_width / 2, s_texture_height / 2);
	result &= CopyTextureSubImage2DAndCheckErrors(m_to_dst, 0, s_texture_width / 2, 0, s_texture_width / 2, 0,
												  s_texture_width / 2, s_texture_height / 2);
	result &= CopyTextureSubImage2DAndCheckErrors(m_to_dst, 0, 0, s_texture_height / 2, 0, s_texture_height / 2,
												  s_texture_width / 2, s_texture_height / 2);
	result &=
		CopyTextureSubImage2DAndCheckErrors(m_to_dst, 0, s_texture_width / 2, s_texture_height / 2, s_texture_width / 2,
											s_texture_height / 2, s_texture_width / 2, s_texture_height / 2);

	result &= CheckData(TextureTarget<2>(), 4 /* RGBA */ * s_texture_width * s_texture_height);

	CleanAll();

	return result;
}

/** @brief Test function */
template <>
bool CopyTest::Test<3>()
{
	CreateAll<3>();

	bool result = true;

	for (glw::GLuint i = 0; i < s_texture_depth; ++i)
	{
		result &=
			CopyTextureSubImage3DAndCheckErrors(m_to_dst, 0, 0, 0, i, 0, 0, s_texture_width / 2, s_texture_height / 2);
		result &= CopyTextureSubImage3DAndCheckErrors(m_to_dst, 0, s_texture_width / 2, 0, i, s_texture_width / 2, 0,
													  s_texture_width / 2, s_texture_height / 2);
		result &= CopyTextureSubImage3DAndCheckErrors(m_to_dst, 0, 0, s_texture_height / 2, i, 0, s_texture_height / 2,
													  s_texture_width / 2, s_texture_height / 2);
		result &= CopyTextureSubImage3DAndCheckErrors(m_to_dst, 0, s_texture_width / 2, s_texture_height / 2, i,
													  s_texture_width / 2, s_texture_height / 2, s_texture_width / 2,
													  s_texture_height / 2);
	}

	result &= CheckData(TextureTarget<3>(), 4 /* RGBA */ * s_texture_width * s_texture_height * s_texture_depth);

	CleanAll();

	return result;
}

/** @brief Compre results with the reference.
 *
 *  @param [in] target      Texture target.
 *  @param [in] size        Size of the buffer.
 *
 *  @return True if equal, false otherwise.
 */
bool CopyTest::CheckData(glw::GLenum target, glw::GLuint size)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check texture content with reference. */
	m_result = new glw::GLubyte[size];

	if (DE_NULL == m_result)
	{
		throw 0;
	}

	gl.getTexImage(target, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_result);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetCompressedTexImage has failed");

	for (glw::GLuint i = 0; i < size; ++i)
	{
		if (s_texture_data[i] != m_result[i])
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "glCopyTextureSubImage*D created texture with data "
				<< DataToString(size, s_texture_data) << " however texture contains data "
				<< DataToString(size, m_result) << ". Texture target was " << glu::getTextureTargetStr(target)
				<< ". Test fails." << tcu::TestLog::EndMessage;

			return false;
		}
	}

	return true;
}

/** @brief Convert raw data into string for logging purposes.
 *
 *  @param [in] count      Count of the data.
 *  @param [in] data       Raw data.
 *
 *  @return String representation of data.
 */
std::string CopyTest::DataToString(glw::GLuint count, const glw::GLubyte data[])
{
	std::string data_str = "[";

	for (glw::GLuint i = 0; i < count; ++i)
	{
		std::stringstream int_sstream;

		int_sstream << unsigned(data[i]);

		data_str.append(int_sstream.str());

		if (i + 1 < count)
		{
			data_str.append(", ");
		}
		else
		{
			data_str.append("]");
		}
	}

	return data_str;
}

/** @brief Clean GL objects, test variables and GL errors.
 */
void CopyTest::CleanAll()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fbo)
	{
		gl.deleteFramebuffers(1, &m_fbo);

		m_fbo = 0;
	}

	if (m_to_src)
	{
		gl.deleteTextures(1, &m_to_src);

		m_to_src = 0;
	}

	if (m_to_dst)
	{
		gl.deleteTextures(1, &m_to_dst);

		m_to_dst = 0;
	}

	if (DE_NULL == m_result)
	{
		delete[] m_result;

		m_result = DE_NULL;
	}

	while (GL_NO_ERROR != gl.getError())
		;
}

/** @brief Iterate Copy Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult CopyTest::iterate()
{
	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		is_ok &= Test<1>();
		is_ok &= Test<2>();
		is_ok &= Test<3>();
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	CleanAll();

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** Reference data. */
const glw::GLubyte CopyTest::s_texture_data[] = {
	0x00, 0x00, 0x00, 0xFF, 0x7f, 0x7f, 0x7f, 0x00, 0xc3, 0xc3, 0xc3, 0xff, 0xff, 0xff, 0xff, 0x00,
	0x88, 0x00, 0x15, 0xFF, 0xed, 0x1c, 0x24, 0x00, 0xff, 0x7f, 0x27, 0xff, 0xff, 0xf2, 0x00, 0x00,
	0xc8, 0xbf, 0xe7, 0xFF, 0x70, 0x92, 0xbe, 0x00, 0x99, 0xd9, 0xea, 0xff, 0xb5, 0xe6, 0x1d, 0x00,
	0xa3, 0x49, 0xa4, 0xFF, 0x3f, 0x48, 0xcc, 0x00, 0x00, 0xa2, 0xe8, 0xff, 0x22, 0xb1, 0x4c, 0x00,

	0xa3, 0x49, 0xa4, 0xFF, 0xc8, 0xbf, 0xe7, 0x00, 0x88, 0x00, 0x15, 0xff, 0x00, 0x00, 0x00, 0x00,
	0x3f, 0x48, 0xcc, 0xFF, 0x70, 0x92, 0xbe, 0x00, 0xed, 0x1c, 0x24, 0xff, 0x7f, 0x7f, 0x7f, 0x00,
	0x00, 0xa2, 0xe8, 0xFF, 0x99, 0xd9, 0xea, 0x00, 0xff, 0x7f, 0x27, 0xff, 0xc3, 0xc3, 0xc3, 0x00,
	0x22, 0xb1, 0x4c, 0xFF, 0xb5, 0xe6, 0x1d, 0x00, 0xff, 0xf2, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00,

	0x22, 0xb1, 0x4c, 0xFF, 0x00, 0xa2, 0xe8, 0x00, 0x3f, 0x48, 0xcc, 0xff, 0xa3, 0x49, 0xa4, 0x00,
	0xb5, 0xe6, 0x1d, 0xFF, 0x99, 0xd9, 0xea, 0x00, 0x70, 0x92, 0xbe, 0xff, 0xc8, 0xbf, 0xe7, 0x00,
	0xff, 0xf2, 0x00, 0xFF, 0xff, 0x7f, 0x27, 0x00, 0xed, 0x1c, 0x24, 0xff, 0x88, 0x00, 0x15, 0x00,
	0xff, 0xff, 0xff, 0xFF, 0xc3, 0xc3, 0xc3, 0x00, 0x7f, 0x7f, 0x7f, 0xff, 0x00, 0x00, 0x00, 0x00,

	0xff, 0xff, 0xff, 0xFF, 0xff, 0xf2, 0x00, 0x00, 0xb5, 0xe6, 0x1d, 0xff, 0x22, 0xb1, 0x4c, 0x00,
	0xc3, 0xc3, 0xc3, 0xFF, 0xff, 0x7f, 0x27, 0x00, 0x99, 0xd9, 0xea, 0xff, 0x00, 0xa2, 0xe8, 0x00,
	0x7f, 0x7f, 0x7f, 0xFF, 0xed, 0x1c, 0x24, 0x00, 0x70, 0x92, 0xbe, 0xff, 0x3f, 0x48, 0xcc, 0x00,
	0x00, 0x00, 0x00, 0xFF, 0x88, 0x00, 0x15, 0x00, 0xc8, 0xbf, 0xe7, 0xff, 0xa3, 0x49, 0xa4, 0x00
};

/** Reference data parameters. */
const glw::GLuint CopyTest::s_texture_width  = 4;
const glw::GLuint CopyTest::s_texture_height = 4;
const glw::GLuint CopyTest::s_texture_depth  = 4;

/******************************** Get Set Parameter Test Implementation   ********************************/

/** @brief Get Set Parameter Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
GetSetParameterTest::GetSetParameterTest(deqp::Context& context)
	: deqp::TestCase(context, "textures_get_set_parameter", "Texture Get Set Parameter Test")
{
	/* Intentionally left blank. */
}

/** @brief Iterate Get Set Parameter Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult GetSetParameterTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	/* Texture. */
	glw::GLuint texture = 0;

	try
	{
		gl.genTextures(1, &texture);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

		gl.bindTexture(GL_TEXTURE_3D, texture);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

		{
			glw::GLenum name	  = GL_DEPTH_STENCIL_TEXTURE_MODE;
			glw::GLint  value_src = GL_DEPTH_COMPONENT;
			glw::GLint  value_dst = 0;

			gl.textureParameteri(texture, name, value_src);
			is_ok &= CheckErrorAndLog("glTextureParameteri", name);

			gl.getTextureParameteriv(texture, name, &value_dst);
			is_ok &= CheckErrorAndLog("glGetTextureParameteriv", name);

			is_ok &= CompareAndLog(value_src, value_dst, name);
		}

		{
			glw::GLenum name	  = GL_TEXTURE_BASE_LEVEL;
			glw::GLint  value_src = 2;
			glw::GLint  value_dst = 0;

			gl.textureParameteri(texture, name, value_src);
			is_ok &= CheckErrorAndLog("glTextureParameteri", name);

			gl.getTextureParameteriv(texture, name, &value_dst);
			is_ok &= CheckErrorAndLog("glGetTextureParameteriv", name);

			is_ok &= CompareAndLog(value_src, value_dst, name);
		}

		{
			glw::GLenum  name		  = GL_TEXTURE_BORDER_COLOR;
			glw::GLfloat value_src[4] = { 0.25, 0.5, 0.75, 1.0 };
			glw::GLfloat value_dst[4] = {};

			gl.textureParameterfv(texture, name, value_src);
			is_ok &= CheckErrorAndLog("glTextureParameterfv", name);

			gl.getTextureParameterfv(texture, name, value_dst);
			is_ok &= CheckErrorAndLog("glGetTextureParameterfv", name);

			is_ok &= CompareAndLog(value_src, value_dst, name);
		}

		{
			glw::GLenum name		 = GL_TEXTURE_BORDER_COLOR;
			glw::GLint  value_src[4] = { 0, 64, -64, -32 };
			glw::GLint  value_dst[4] = {};

			gl.textureParameterIiv(texture, name, value_src);
			is_ok &= CheckErrorAndLog("glTextureParameterIiv", name);

			gl.getTextureParameterIiv(texture, name, value_dst);
			is_ok &= CheckErrorAndLog("glGetTextureParameterIiv", name);

			is_ok &= CompareAndLog(value_src, value_dst, name);
		}

		{
			glw::GLenum name		 = GL_TEXTURE_BORDER_COLOR;
			glw::GLuint value_src[4] = { 0, 64, 128, 192 };
			glw::GLuint value_dst[4] = {};

			gl.textureParameterIuiv(texture, name, value_src);
			is_ok &= CheckErrorAndLog("glTextureParameterIuiv", name);

			gl.getTextureParameterIuiv(texture, name, value_dst);
			is_ok &= CheckErrorAndLog("glGetTextureParameterIuiv", name);

			is_ok &= CompareAndLog(value_src, value_dst, name);
		}

		{
			glw::GLenum name	  = GL_TEXTURE_COMPARE_FUNC;
			glw::GLint  value_src = GL_LEQUAL;
			glw::GLint  value_dst = 0;

			gl.textureParameteri(texture, name, value_src);
			is_ok &= CheckErrorAndLog("glTextureParameteri", name);

			gl.getTextureParameteriv(texture, name, &value_dst);
			is_ok &= CheckErrorAndLog("glGetTextureParameteriv", name);

			is_ok &= CompareAndLog(value_src, value_dst, name);
		}

		{
			glw::GLenum name	  = GL_TEXTURE_COMPARE_MODE;
			glw::GLint  value_src = GL_COMPARE_REF_TO_TEXTURE;
			glw::GLint  value_dst = 0;

			gl.textureParameteri(texture, name, value_src);
			is_ok &= CheckErrorAndLog("glTextureParameteri", name);

			gl.getTextureParameteriv(texture, name, &value_dst);
			is_ok &= CheckErrorAndLog("glGetTextureParameteriv", name);

			is_ok &= CompareAndLog(value_src, value_dst, name);
		}

		{
			glw::GLenum  name	  = GL_TEXTURE_LOD_BIAS;
			glw::GLfloat value_src = -2.f;
			glw::GLfloat value_dst = 0.f;

			gl.textureParameterf(texture, name, value_src);
			is_ok &= CheckErrorAndLog("glTextureParameterf", name);

			gl.getTextureParameterfv(texture, name, &value_dst);
			is_ok &= CheckErrorAndLog("glGetTextureParameterfv", name);

			is_ok &= CompareAndLog(value_src, value_dst, name);
		}

		{
			glw::GLenum name	  = GL_TEXTURE_MIN_FILTER;
			glw::GLint  value_src = GL_LINEAR_MIPMAP_NEAREST;
			glw::GLint  value_dst = 0;

			gl.textureParameteri(texture, name, value_src);
			is_ok &= CheckErrorAndLog("glTextureParameteri", name);

			gl.getTextureParameteriv(texture, name, &value_dst);
			is_ok &= CheckErrorAndLog("glGetTextureParameteriv", name);

			is_ok &= CompareAndLog(value_src, value_dst, name);
		}

		{
			glw::GLenum name	  = GL_TEXTURE_MAG_FILTER;
			glw::GLint  value_src = GL_NEAREST;
			glw::GLint  value_dst = 0;

			gl.textureParameteri(texture, name, value_src);
			is_ok &= CheckErrorAndLog("glTextureParameteri", name);

			gl.getTextureParameteriv(texture, name, &value_dst);
			is_ok &= CheckErrorAndLog("glGetTextureParameteriv", name);

			is_ok &= CompareAndLog(value_src, value_dst, name);
		}

		{
			glw::GLenum name	  = GL_TEXTURE_MIN_LOD;
			glw::GLint  value_src = -100;
			glw::GLint  value_dst = 0;

			gl.textureParameteri(texture, name, value_src);
			is_ok &= CheckErrorAndLog("glTextureParameteri", name);

			gl.getTextureParameteriv(texture, name, &value_dst);
			is_ok &= CheckErrorAndLog("glGetTextureParameteriv", name);

			is_ok &= CompareAndLog(value_src, value_dst, name);
		}

		{
			glw::GLenum name	  = GL_TEXTURE_MAX_LOD;
			glw::GLint  value_src = 100;
			glw::GLint  value_dst = 0;

			gl.textureParameteri(texture, name, value_src);
			is_ok &= CheckErrorAndLog("glTextureParameteri", name);

			gl.getTextureParameteriv(texture, name, &value_dst);
			is_ok &= CheckErrorAndLog("glGetTextureParameteriv", name);

			is_ok &= CompareAndLog(value_src, value_dst, name);
		}

		{
			glw::GLenum name	  = GL_TEXTURE_MAX_LEVEL;
			glw::GLint  value_src = 100;
			glw::GLint  value_dst = 0;

			gl.textureParameteri(texture, name, value_src);
			is_ok &= CheckErrorAndLog("glTextureParameteri", name);

			gl.getTextureParameteriv(texture, name, &value_dst);
			is_ok &= CheckErrorAndLog("glGetTextureParameteriv", name);

			is_ok &= CompareAndLog(value_src, value_dst, name);
		}

		{
			glw::GLenum name	  = GL_TEXTURE_SWIZZLE_R;
			glw::GLint  value_src = GL_BLUE;
			glw::GLint  value_dst = 0;

			gl.textureParameteri(texture, name, value_src);
			is_ok &= CheckErrorAndLog("glTextureParameteri", name);

			gl.getTextureParameteriv(texture, name, &value_dst);
			is_ok &= CheckErrorAndLog("glGetTextureParameteriv", name);

			is_ok &= CompareAndLog(value_src, value_dst, name);
		}

		{
			glw::GLenum name	  = GL_TEXTURE_SWIZZLE_G;
			glw::GLint  value_src = GL_ALPHA;
			glw::GLint  value_dst = 0;

			gl.textureParameteri(texture, name, value_src);
			is_ok &= CheckErrorAndLog("glTextureParameteri", name);

			gl.getTextureParameteriv(texture, name, &value_dst);
			is_ok &= CheckErrorAndLog("glGetTextureParameteriv", name);

			is_ok &= CompareAndLog(value_src, value_dst, name);
		}

		{
			glw::GLenum name	  = GL_TEXTURE_SWIZZLE_B;
			glw::GLint  value_src = GL_RED;
			glw::GLint  value_dst = 0;

			gl.textureParameteri(texture, name, value_src);
			is_ok &= CheckErrorAndLog("glTextureParameteri", name);

			gl.getTextureParameteriv(texture, name, &value_dst);
			is_ok &= CheckErrorAndLog("glGetTextureParameteriv", name);

			is_ok &= CompareAndLog(value_src, value_dst, name);
		}

		{
			glw::GLenum name	  = GL_TEXTURE_SWIZZLE_A;
			glw::GLint  value_src = GL_GREEN;
			glw::GLint  value_dst = 0;

			gl.textureParameteri(texture, name, value_src);
			is_ok &= CheckErrorAndLog("glTextureParameteri", name);

			gl.getTextureParameteriv(texture, name, &value_dst);
			is_ok &= CheckErrorAndLog("glGetTextureParameteriv", name);

			is_ok &= CompareAndLog(value_src, value_dst, name);
		}

		{
			glw::GLenum name		 = GL_TEXTURE_SWIZZLE_RGBA;
			glw::GLint  value_src[4] = { GL_ZERO, GL_ONE, GL_ZERO, GL_ONE };
			glw::GLint  value_dst[4] = {};

			gl.textureParameteriv(texture, name, value_src);
			is_ok &= CheckErrorAndLog("glTextureParameteri", name);

			gl.getTextureParameteriv(texture, name, value_dst);
			is_ok &= CheckErrorAndLog("glGetTextureParameteriv", name);

			is_ok &= CompareAndLog(value_src, value_dst, name);
		}

		{
			glw::GLenum name	  = GL_TEXTURE_WRAP_S;
			glw::GLint  value_src = GL_MIRROR_CLAMP_TO_EDGE;
			glw::GLint  value_dst = 11;

			gl.textureParameteri(texture, name, value_src);
			is_ok &= CheckErrorAndLog("glTextureParameteri", name);

			gl.getTextureParameteriv(texture, name, &value_dst);
			is_ok &= CheckErrorAndLog("glGetTextureParameteriv", name);

			is_ok &= CompareAndLog(value_src, value_dst, name);
		}

		{
			glw::GLenum name	  = GL_TEXTURE_WRAP_T;
			glw::GLint  value_src = GL_CLAMP_TO_EDGE;
			glw::GLint  value_dst = 11;

			gl.textureParameteri(texture, name, value_src);
			is_ok &= CheckErrorAndLog("glTextureParameteri", name);

			gl.getTextureParameteriv(texture, name, &value_dst);
			is_ok &= CheckErrorAndLog("glGetTextureParameteriv", name);

			is_ok &= CompareAndLog(value_src, value_dst, name);
		}

		{
			glw::GLenum name	  = GL_TEXTURE_WRAP_R;
			glw::GLint  value_src = GL_CLAMP_TO_EDGE;
			glw::GLint  value_dst = 11;

			gl.textureParameteriv(texture, name, &value_src);
			is_ok &= CheckErrorAndLog("glTextureParameteri", name);

			gl.getTextureParameteriv(texture, name, &value_dst);
			is_ok &= CheckErrorAndLog("glGetTextureParameteriv", name);

			is_ok &= CompareAndLog(value_src, value_dst, name);
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	if (texture)
	{
		gl.deleteTextures(1, &texture);
	}

	while (GL_NO_ERROR != gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** @brief Check for errors and log.
 *
 *  @param [in] fname     Name of the function (to be logged).
 *  @param [in] pname     Parameter name with which function was called.
 *
 *  @return True if no error, false otherwise.
 */
bool GetSetParameterTest::CheckErrorAndLog(const glw::GLchar* fname, glw::GLenum pname)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check errors. */
	glw::GLenum error;

	if (GL_NO_ERROR != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << fname << " unexpectedly generated error "
											<< glu::getErrorStr(error) << " during test of pname "
											<< glu::getTextureParameterStr(pname) << ". Test fails."
											<< tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Compare queried value of parameter with the expected vale.
 *
 *  @param [in] value_src           First value.
 *  @param [in] value_dst           Second value.
 *  @param [in] pname               Parameter name.
 *
 *  @return True if no error was generated by CopyTextureSubImage*D, false otherwise
 */
bool GetSetParameterTest::CompareAndLog(glw::GLint value_src, glw::GLint value_dst, glw::GLenum pname)
{
	if (value_src != value_dst)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Queried value of pname "
											<< glu::getTextureParameterStr(pname) << " is equal to " << value_dst
											<< ", however " << value_src << " was expected. Test fails."
											<< tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Compare queried value of parameter with the expected vale.
 *
 *  @param [in] value_src           First value.
 *  @param [in] value_dst           Second value.
 *  @param [in] pname               Parameter name.
 *
 *  @return True if no error was generated by CopyTextureSubImage*D, false otherwise
 */
bool GetSetParameterTest::CompareAndLog(glw::GLuint value_src, glw::GLuint value_dst, glw::GLenum pname)
{
	if (value_src != value_dst)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Queried value of pname "
											<< glu::getTextureParameterStr(pname) << " is equal to " << value_dst
											<< ", however " << value_src << " was expected. Test fails."
											<< tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Compare queried value of parameter with the expected vale.
 *
 *  @param [in] value_src           First value.
 *  @param [in] value_dst           Second value.
 *  @param [in] pname               Parameter name.
 *
 *  @return True if no error was generated by CopyTextureSubImage*D, false otherwise
 */
bool GetSetParameterTest::CompareAndLog(glw::GLfloat value_src, glw::GLfloat value_dst, glw::GLenum pname)
{
	if (de::abs(value_src - value_dst) > 0.0125 /* Precision */)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Queried value of pname "
											<< glu::getTextureParameterStr(pname) << " is equal to " << value_dst
											<< ", however " << value_src << " was expected. Test fails."
											<< tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Compare queried value of parameter with the expected vale.
 *
 *  @param [in] value_src           First value.
 *  @param [in] value_dst           Second value.
 *  @param [in] pname               Parameter name.
 *
 *  @return True if no error was generated by CopyTextureSubImage*D, false otherwise
 */
bool GetSetParameterTest::CompareAndLog(glw::GLint value_src[4], glw::GLint value_dst[4], glw::GLenum pname)
{
	if ((value_src[0] != value_dst[0]) || (value_src[1] != value_dst[1]) || (value_src[2] != value_dst[2]) ||
		(value_src[3] != value_dst[3]))
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Queried value of pname " << glu::getTextureParameterStr(pname)
			<< " is equal to [" << value_dst[0] << ", " << value_dst[1] << ", " << value_dst[2] << ", " << value_dst[3]
			<< "], however " << value_src[0] << ", " << value_src[1] << ", " << value_src[2] << ", " << value_src[3]
			<< "] was expected. Test fails." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Compare queried value of parameter with the expected vale.
 *
 *  @param [in] value_src           First value.
 *  @param [in] value_dst           Second value.
 *  @param [in] pname               Parameter name.
 *
 *  @return True if no error was generated by CopyTextureSubImage*D, false otherwise
 */
bool GetSetParameterTest::CompareAndLog(glw::GLuint value_src[4], glw::GLuint value_dst[4], glw::GLenum pname)
{
	if ((value_src[0] != value_dst[0]) || (value_src[1] != value_dst[1]) || (value_src[2] != value_dst[2]) ||
		(value_src[3] != value_dst[3]))
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Queried value of pname " << glu::getTextureParameterStr(pname)
			<< " is equal to [" << value_dst[0] << ", " << value_dst[1] << ", " << value_dst[2] << ", " << value_dst[3]
			<< "], however " << value_src[0] << ", " << value_src[1] << ", " << value_src[2] << ", " << value_src[3]
			<< "] was expected. Test fails." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Compare queried value of parameter with the expected vale.
 *
 *  @param [in] value_src           First value.
 *  @param [in] value_dst           Second value.
 *  @param [in] pname               Parameter name.
 *
 *  @return True if no error was generated by CopyTextureSubImage*D, false otherwise
 */
bool GetSetParameterTest::CompareAndLog(glw::GLfloat value_src[4], glw::GLfloat value_dst[4], glw::GLenum pname)
{
	if ((de::abs(value_src[0] - value_dst[0]) > 0.0125 /* Precision */) ||
		(de::abs(value_src[1] - value_dst[1]) > 0.0125 /* Precision */) ||
		(de::abs(value_src[2] - value_dst[2]) > 0.0125 /* Precision */) ||
		(de::abs(value_src[3] - value_dst[3]) > 0.0125 /* Precision */))
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Queried value of pname " << glu::getTextureParameterStr(pname)
			<< " is equal to [" << value_dst[0] << ", " << value_dst[1] << ", " << value_dst[2] << ", " << value_dst[3]
			<< "], however " << value_src[0] << ", " << value_src[1] << ", " << value_src[2] << ", " << value_src[3]
			<< "] was expected. Test fails." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/******************************** Defaults Test Implementation   ********************************/

/** @brief Defaults Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
DefaultsTest::DefaultsTest(deqp::Context& context)
	: deqp::TestCase(context, "textures_defaults", "Texture Defaults Test")
{
	/* Intentionally left blank. */
}

/** @brief Defaults Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult DefaultsTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	/* Texture. */
	glw::GLuint texture = 0;

	try
	{
		gl.createTextures(GL_TEXTURE_3D, 1, &texture);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

		{
			glw::GLenum name	  = GL_DEPTH_STENCIL_TEXTURE_MODE;
			glw::GLint  value_ref = GL_DEPTH_COMPONENT;
			glw::GLint  value_dst = 0;

			gl.getTextureParameteriv(texture, name, &value_dst);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTextureParameter has failed");

			is_ok &= CompareAndLog(value_ref, value_dst, name);
		}

		{
			glw::GLenum name	  = GL_TEXTURE_BASE_LEVEL;
			glw::GLint  value_ref = 0;
			glw::GLint  value_dst = 1;

			gl.getTextureParameteriv(texture, name, &value_dst);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTextureParameter has failed");

			is_ok &= CompareAndLog(value_ref, value_dst, name);
		}

		{
			glw::GLenum  name		  = GL_TEXTURE_BORDER_COLOR;
			glw::GLfloat value_ref[4] = { 0.f, 0.f, 0.f, 0.f };
			glw::GLfloat value_dst[4] = {};

			gl.getTextureParameterfv(texture, name, value_dst);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTextureParameter has failed");

			is_ok &= CompareAndLog(value_ref, value_dst, name);
		}

		{
			glw::GLenum name	  = GL_TEXTURE_COMPARE_FUNC;
			glw::GLint  value_ref = GL_LEQUAL;
			glw::GLint  value_dst = 0;

			gl.getTextureParameteriv(texture, name, &value_dst);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTextureParameter has failed");

			is_ok &= CompareAndLog(value_ref, value_dst, name);
		}

		{
			glw::GLenum name	  = GL_TEXTURE_COMPARE_MODE;
			glw::GLint  value_ref = GL_NONE;
			glw::GLint  value_dst = 0;

			gl.getTextureParameteriv(texture, name, &value_dst);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTextureParameter has failed");

			is_ok &= CompareAndLog(value_ref, value_dst, name);
		}

		{
			glw::GLenum  name	  = GL_TEXTURE_LOD_BIAS;
			glw::GLfloat value_ref = 0.f;
			glw::GLfloat value_dst = 0.f;

			gl.getTextureParameterfv(texture, name, &value_dst);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTextureParameter has failed");

			is_ok &= CompareAndLog(value_ref, value_dst, name);
		}

		{
			glw::GLenum name	  = GL_TEXTURE_MIN_FILTER;
			glw::GLint  value_ref = GL_NEAREST_MIPMAP_LINEAR;
			glw::GLint  value_dst = 0;

			gl.getTextureParameteriv(texture, name, &value_dst);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTextureParameter has failed");

			is_ok &= CompareAndLog(value_ref, value_dst, name);
		}

		{
			glw::GLenum name	  = GL_TEXTURE_MAG_FILTER;
			glw::GLint  value_ref = GL_LINEAR;
			glw::GLint  value_dst = 0;

			gl.getTextureParameteriv(texture, name, &value_dst);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTextureParameter has failed");

			is_ok &= CompareAndLog(value_ref, value_dst, name);
		}

		{
			glw::GLenum name	  = GL_TEXTURE_MIN_LOD;
			glw::GLint  value_ref = -1000;
			glw::GLint  value_dst = 0;

			gl.getTextureParameteriv(texture, name, &value_dst);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTextureParameter has failed");

			is_ok &= CompareAndLog(value_ref, value_dst, name);
		}

		{
			glw::GLenum name	  = GL_TEXTURE_MAX_LOD;
			glw::GLint  value_ref = 1000;
			glw::GLint  value_dst = 0;

			gl.getTextureParameteriv(texture, name, &value_dst);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTextureParameter has failed");

			is_ok &= CompareAndLog(value_ref, value_dst, name);
		}

		{
			glw::GLenum name	  = GL_TEXTURE_MAX_LEVEL;
			glw::GLint  value_ref = 1000;
			glw::GLint  value_dst = 0;

			gl.getTextureParameteriv(texture, name, &value_dst);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTextureParameter has failed");

			is_ok &= CompareAndLog(value_ref, value_dst, name);
		}

		{
			glw::GLenum name	  = GL_TEXTURE_SWIZZLE_R;
			glw::GLint  value_ref = GL_RED;
			glw::GLint  value_dst = 0;

			gl.getTextureParameteriv(texture, name, &value_dst);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTextureParameter has failed");

			is_ok &= CompareAndLog(value_ref, value_dst, name);
		}

		{
			glw::GLenum name	  = GL_TEXTURE_SWIZZLE_G;
			glw::GLint  value_ref = GL_GREEN;
			glw::GLint  value_dst = 0;

			gl.getTextureParameteriv(texture, name, &value_dst);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTextureParameter has failed");

			is_ok &= CompareAndLog(value_ref, value_dst, name);
		}

		{
			glw::GLenum name	  = GL_TEXTURE_SWIZZLE_B;
			glw::GLint  value_ref = GL_BLUE;
			glw::GLint  value_dst = 0;

			gl.getTextureParameteriv(texture, name, &value_dst);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTextureParameter has failed");

			is_ok &= CompareAndLog(value_ref, value_dst, name);
		}

		{
			glw::GLenum name	  = GL_TEXTURE_SWIZZLE_A;
			glw::GLint  value_ref = GL_ALPHA;
			glw::GLint  value_dst = 0;

			gl.getTextureParameteriv(texture, name, &value_dst);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTextureParameter has failed");

			is_ok &= CompareAndLog(value_ref, value_dst, name);
		}

		{
			glw::GLenum name	  = GL_TEXTURE_WRAP_S;
			glw::GLint  value_ref = GL_REPEAT;
			glw::GLint  value_dst = 11;

			gl.getTextureParameteriv(texture, name, &value_dst);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTextureParameter has failed");

			is_ok &= CompareAndLog(value_ref, value_dst, name);
		}

		{
			glw::GLenum name	  = GL_TEXTURE_WRAP_T;
			glw::GLint  value_ref = GL_REPEAT;
			glw::GLint  value_dst = 11;

			gl.getTextureParameteriv(texture, name, &value_dst);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTextureParameter has failed");

			is_ok &= CompareAndLog(value_ref, value_dst, name);
		}

		{
			glw::GLenum name	  = GL_TEXTURE_WRAP_R;
			glw::GLint  value_ref = GL_REPEAT;
			glw::GLint  value_dst = 11;

			gl.getTextureParameteriv(texture, name, &value_dst);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTextureParameter has failed");

			is_ok &= CompareAndLog(value_ref, value_dst, name);
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	if (texture)
	{
		gl.deleteTextures(1, &texture);
	}

	while (GL_NO_ERROR != gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** @brief Compare queried value of parameter with the expected vale.
 *
 *  @param [in] value_src           First value.
 *  @param [in] value_dst           Second value.
 *  @param [in] pname               Parameter name.
 *
 *  @return True if no error was generated by CopyTextureSubImage*D, false otherwise
 */
bool DefaultsTest::CompareAndLog(glw::GLint value_ref, glw::GLint value_dst, glw::GLenum pname)
{
	if (value_ref != value_dst)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Queried value of pname "
											<< glu::getTextureParameterStr(pname) << " is equal to " << value_dst
											<< ", however " << value_ref << " was expected. Test fails."
											<< tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Compare queried value of parameter with the expected vale.
 *
 *  @param [in] value_src           First value.
 *  @param [in] value_dst           Second value.
 *  @param [in] pname               Parameter name.
 *
 *  @return True if no error was generated by CopyTextureSubImage*D, false otherwise
 */
bool DefaultsTest::CompareAndLog(glw::GLuint value_ref, glw::GLuint value_dst, glw::GLenum pname)
{
	if (value_ref != value_dst)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Queried value of pname "
											<< glu::getTextureParameterStr(pname) << " is equal to " << value_dst
											<< ", however " << value_ref << " was expected. Test fails."
											<< tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Compare queried value of parameter with the expected vale.
 *
 *  @param [in] value_src           First value.
 *  @param [in] value_dst           Second value.
 *  @param [in] pname               Parameter name.
 *
 *  @return True if no error was generated by CopyTextureSubImage*D, false otherwise
 */
bool DefaultsTest::CompareAndLog(glw::GLfloat value_ref, glw::GLfloat value_dst, glw::GLenum pname)
{
	if (de::abs(value_ref - value_dst) > 0.0125 /* Precision */)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Queried value of pname "
											<< glu::getTextureParameterStr(pname) << " is equal to " << value_dst
											<< ", however " << value_ref << " was expected. Test fails."
											<< tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Compare queried value of parameter with the expected vale.
 *
 *  @param [in] value_src           First value.
 *  @param [in] value_dst           Second value.
 *  @param [in] pname               Parameter name.
 *
 *  @return True if no error was generated by CopyTextureSubImage*D, false otherwise
 */
bool DefaultsTest::CompareAndLog(glw::GLint value_ref[4], glw::GLint value_dst[4], glw::GLenum pname)
{
	if ((value_ref[0] != value_dst[0]) || (value_ref[1] != value_dst[1]) || (value_ref[2] != value_dst[2]) ||
		(value_ref[3] != value_dst[3]))
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Queried value of pname " << glu::getTextureParameterStr(pname)
			<< " is equal to [" << value_dst[0] << ", " << value_dst[1] << ", " << value_dst[2] << ", " << value_dst[3]
			<< "], however " << value_ref[0] << ", " << value_ref[1] << ", " << value_ref[2] << ", " << value_ref[3]
			<< "] was expected. Test fails." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Compare queried value of parameter with the expected vale.
 *
 *  @param [in] value_src           First value.
 *  @param [in] value_dst           Second value.
 *  @param [in] pname               Parameter name.
 *
 *  @return True if no error was generated by CopyTextureSubImage*D, false otherwise
 */
bool DefaultsTest::CompareAndLog(glw::GLuint value_ref[4], glw::GLuint value_dst[4], glw::GLenum pname)
{
	if ((value_ref[0] != value_dst[0]) || (value_ref[1] != value_dst[1]) || (value_ref[2] != value_dst[2]) ||
		(value_ref[3] != value_dst[3]))
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Queried value of pname " << glu::getTextureParameterStr(pname)
			<< " is equal to [" << value_dst[0] << ", " << value_dst[1] << ", " << value_dst[2] << ", " << value_dst[3]
			<< "], however " << value_ref[0] << ", " << value_ref[1] << ", " << value_ref[2] << ", " << value_ref[3]
			<< "] was expected. Test fails." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Compare queried value of parameter with the expected vale.
 *
 *  @param [in] value_src           First value.
 *  @param [in] value_dst           Second value.
 *  @param [in] pname               Parameter name.
 *
 *  @return True if no error was generated by CopyTextureSubImage*D, false otherwise
 */
bool DefaultsTest::CompareAndLog(glw::GLfloat value_ref[4], glw::GLfloat value_dst[4], glw::GLenum pname)
{
	if ((de::abs(value_ref[0] - value_dst[0]) > 0.0125 /* Precision */) ||
		(de::abs(value_ref[1] - value_dst[1]) > 0.0125 /* Precision */) ||
		(de::abs(value_ref[2] - value_dst[2]) > 0.0125 /* Precision */) ||
		(de::abs(value_ref[3] - value_dst[3]) > 0.0125 /* Precision */))
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Queried value of pname " << glu::getTextureParameterStr(pname)
			<< " is equal to [" << value_dst[0] << ", " << value_dst[1] << ", " << value_dst[2] << ", " << value_dst[3]
			<< "], however " << value_ref[0] << ", " << value_ref[1] << ", " << value_ref[2] << ", " << value_ref[3]
			<< "] was expected. Test fails." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/******************************** Generate Mipmap Test Implementation   ********************************/

/** @brief Generate Mipmap Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
GenerateMipmapTest::GenerateMipmapTest(deqp::Context& context)
	: deqp::TestCase(context, "textures_generate_mipmaps", "Textures Generate Mipmap Test")
{
	/* Intentionally left blank. */
}

/** @brief Generate Mipmap Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult GenerateMipmapTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	/* Texture and cpu results storage. */
	glw::GLuint   texture = 0;
	glw::GLubyte* result  = DE_NULL;

	try
	{
		/* Prepare texture. */
		gl.genTextures(1, &texture);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

		gl.bindTexture(GL_TEXTURE_1D, texture);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

		gl.texImage1D(GL_TEXTURE_1D, 0, GL_R8, s_texture_width, 0, GL_RED, GL_UNSIGNED_BYTE, s_texture_data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage1D has failed");

		/* Generate mipmaps with tested function. */
		gl.generateTextureMipmap(texture);

		glw::GLenum error = GL_NO_ERROR;

		if (GL_NO_ERROR != (error = gl.getError()))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GenerateTextureMipmap unexpectedly generated error "
				<< glu::getErrorStr(error) << ". Test fails." << tcu::TestLog::EndMessage;

			is_ok = false;
		}

		/* Continue only if mipmaps has been generated. */
		if (is_ok)
		{
			result = new glw::GLubyte[s_texture_width];

			if (DE_NULL == result)
			{
				throw 0;
			}

			/* For each mipmap. */
			for (glw::GLuint i = 0, j = s_texture_width;
				 i < s_texture_width_log - 1 /* Do not test single pixel mipmap. */; ++i, j /= 2)
			{
				/* Check mipmap size. */
				glw::GLint mipmap_size = 0;

				gl.getTexLevelParameteriv(GL_TEXTURE_1D, i, GL_TEXTURE_WIDTH, &mipmap_size);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameteriv has failed");

				if (mipmap_size != (glw::GLint)j)
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message
						<< "GenerateTextureMipmap unexpectedly generated mipmap with improper size. Mipmap size is "
						<< mipmap_size << ", but " << j << " was expected. Test fails." << tcu::TestLog::EndMessage;

					is_ok = false;

					break;
				}

				/* Fetch data. */
				gl.getTexImage(GL_TEXTURE_1D, i, GL_RED, GL_UNSIGNED_BYTE, result);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexImage has failed");

				/* Make comparison. */
				for (glw::GLuint k = 0; k < j - 1; ++k)
				{
					if (((glw::GLint)result[k + 1]) - ((glw::GLint)result[k]) < 0)
					{
						m_context.getTestContext().getLog() << tcu::TestLog::Message
															<< "GenerateTextureMipmap unexpectedly generated improper "
															   "mipmap (not descending). Test fails."
															<< tcu::TestLog::EndMessage;

						is_ok = false;

						break;
					}
				}
			}
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	if (texture)
	{
		gl.deleteTextures(1, &texture);
	}

	if (DE_NULL != result)
	{
		delete[] result;
	}

	while (GL_NO_ERROR != gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** Reference data. */
const glw::GLubyte GenerateMipmapTest::s_texture_data[] = {
	0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,
	22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,
	44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,
	66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,
	88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  99,  100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
	110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131,
	132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153,
	154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
	176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197,
	198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219,
	220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241,
	242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
};

/** Reference data parameters. */
const glw::GLuint GenerateMipmapTest::s_texture_width	 = 256;
const glw::GLuint GenerateMipmapTest::s_texture_width_log = 8;

/******************************** Bind Unit Test Implementation   ********************************/

/** @brief Bind Unit Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
BindUnitTest::BindUnitTest(deqp::Context& context)
	: deqp::TestCase(context, "textures_bind_unit", "Textures Bind Unit Test")
	, m_po(0)
	, m_fbo(0)
	, m_rbo(0)
	, m_vao(0)
	, m_result(DE_NULL)
{
	m_to[0] = 0;
	m_to[1] = 0;
	m_to[2] = 0;
	m_to[3] = 0;
}

/** @brief Bind Unit Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult BindUnitTest::iterate()
{
	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		CreateProgram();
		CreateTextures();
		CreateFrambuffer();
		CreateVertexArray();
		is_ok &= Draw();
		is_ok &= Check();
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	CleanAll();

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** @brief Create test program.
 */
void BindUnitTest::CreateProgram()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	struct Shader
	{
		glw::GLchar const* source;
		glw::GLenum const  type;
		glw::GLuint		   id;
	} shader[] = { { s_vertex_shader, GL_VERTEX_SHADER, 0 }, { s_fragment_shader, GL_FRAGMENT_SHADER, 0 } };

	glw::GLuint const shader_count = sizeof(shader) / sizeof(shader[0]);

	try
	{
		/* Create program. */
		m_po = gl.createProgram();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram call failed.");

		/* Shader compilation. */

		for (glw::GLuint i = 0; i < shader_count; ++i)
		{
			if (DE_NULL != shader[i].source)
			{
				shader[i].id = gl.createShader(shader[i].type);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader call failed.");

				gl.attachShader(m_po, shader[i].id);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader call failed.");

				gl.shaderSource(shader[i].id, 1, &shader[i].source, NULL);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource call failed.");

				gl.compileShader(shader[i].id);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader call failed.");

				glw::GLint status = GL_FALSE;

				gl.getShaderiv(shader[i].id, GL_COMPILE_STATUS, &status);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv call failed.");

				if (GL_FALSE == status)
				{
					glw::GLint log_size = 0;
					gl.getShaderiv(shader[i].id, GL_INFO_LOG_LENGTH, &log_size);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv call failed.");

					glw::GLchar* log_text = new glw::GLchar[log_size];

					gl.getShaderInfoLog(shader[i].id, log_size, NULL, &log_text[0]);

					m_context.getTestContext().getLog() << tcu::TestLog::Message << "Shader compilation has failed.\n"
														<< "Shader type: " << glu::getShaderTypeStr(shader[i].type)
														<< "\n"
														<< "Shader compilation error log:\n"
														<< log_text << "\n"
														<< "Shader source code:\n"
														<< shader[i].source << "\n"
														<< tcu::TestLog::EndMessage;

					delete[] log_text;

					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderInfoLog call failed.");

					throw 0;
				}
			}
		}

		/* Link. */
		gl.linkProgram(m_po);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings call failed.");

		glw::GLint status = GL_FALSE;

		gl.getProgramiv(m_po, GL_LINK_STATUS, &status);

		if (GL_TRUE == status)
		{
			for (glw::GLuint i = 0; i < shader_count; ++i)
			{
				if (shader[i].id)
				{
					gl.detachShader(m_po, shader[i].id);

					GLU_EXPECT_NO_ERROR(gl.getError(), "glDetachShader call failed.");
				}
			}
		}
		else
		{
			glw::GLint log_size = 0;

			gl.getProgramiv(m_po, GL_INFO_LOG_LENGTH, &log_size);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv call failed.");

			glw::GLchar* log_text = new glw::GLchar[log_size];

			gl.getProgramInfoLog(m_po, log_size, NULL, &log_text[0]);

			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Program linkage has failed due to:\n"
												<< log_text << "\n"
												<< tcu::TestLog::EndMessage;

			delete[] log_text;

			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramInfoLog call failed.");

			throw 0;
		}
	}
	catch (...)
	{
		if (m_po)
		{
			gl.deleteProgram(m_po);

			m_po = 0;
		}
	}

	for (glw::GLuint i = 0; i < shader_count; ++i)
	{
		if (0 != shader[i].id)
		{
			gl.deleteShader(shader[i].id);

			shader[i].id = 0;
		}
	}

	if (0 == m_po)
	{
		throw 0;
	}
}

/** @brief Create texture.
 */
void BindUnitTest::CreateTextures()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Prepare texture. */
	gl.genTextures(4, m_to);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

	/* Setup pixel sotre modes.*/
	gl.pixelStorei(GL_UNPACK_ALIGNMENT, sizeof(glw::GLubyte));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPixelStorei has failed");

	gl.pixelStorei(GL_PACK_ALIGNMENT, sizeof(glw::GLubyte));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPixelStorei has failed");

	/* Red texture. */
	gl.bindTexture(GL_TEXTURE_2D, m_to[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

	gl.texImage2D(GL_TEXTURE_2D, 0, GL_R8, s_texture_width, s_texture_height, 0, GL_RED, GL_UNSIGNED_BYTE,
				  s_texture_data_r);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage1D has failed");

	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri call failed.");

	/* Green texture. */
	gl.bindTexture(GL_TEXTURE_2D, m_to[1]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

	gl.texImage2D(GL_TEXTURE_2D, 0, GL_R8, s_texture_width, s_texture_height, 0, GL_RED, GL_UNSIGNED_BYTE,
				  s_texture_data_g);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage1D has failed");

	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri call failed.");

	/* Blue texture. */
	gl.bindTexture(GL_TEXTURE_2D, m_to[2]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

	gl.texImage2D(GL_TEXTURE_2D, 0, GL_R8, s_texture_width, s_texture_height, 0, GL_RED, GL_UNSIGNED_BYTE,
				  s_texture_data_b);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage1D has failed");

	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri call failed.");

	/* Alpha texture. */
	gl.bindTexture(GL_TEXTURE_2D, m_to[3]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

	gl.texImage2D(GL_TEXTURE_2D, 0, GL_R8, s_texture_width, s_texture_height, 0, GL_RED, GL_UNSIGNED_BYTE,
				  s_texture_data_a);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage1D has failed");

	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri call failed.");
}

/** @brief Create framebuffer.
 */
void BindUnitTest::CreateFrambuffer()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Prepare framebuffer. */
	gl.genFramebuffers(1, &m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers call failed.");

	gl.genRenderbuffers(1, &m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer call failed.");

	gl.renderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, s_texture_width, s_texture_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage call failed.");

	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer call failed.");

	if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		throw 0;
	}

	gl.viewport(0, 0, s_texture_width, s_texture_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport call failed.");

	/* Clear framebuffer's content. */
	gl.clearColor(0.0f, 0.0f, 0.0f, 0.0f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor call failed.");

	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear call failed.");
}

/** @brief Create vertex array object.
 */
void BindUnitTest::CreateVertexArray()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genVertexArrays(1, &m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays call has failed.");

	gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call has failed.");
}

bool BindUnitTest::Draw()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Setup program. */
	gl.useProgram(m_po);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call has failed.");

	/* Bind textures to proper units and setup program's samplers. */
	for (glw::GLuint i = 0; i < 4; ++i)
	{
		/* Tested binding funcion. */
		gl.bindTextureUnit(i, m_to[i]);

		/* Check for errors. */
		glw::GLenum error = GL_NO_ERROR;

		if (GL_NO_ERROR != (error = gl.getError()))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "BindTextureUnit unexpectedly generated error " << glu::getErrorStr(error)
				<< " when binding texture " << m_to[i] << " to texture unit " << i << ". Test fails."
				<< tcu::TestLog::EndMessage;

			return false;
		}

		/* Sampler setup. */
		gl.uniform1i(gl.getUniformLocation(m_po, s_fragment_shader_samplers[i]), i);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformLocation or glUniform1i call has failed.");
	}

	/* Draw call. */
	gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays call has failed.");

	return true;
}

/** @brief Compare results with reference.
 *
 *  @return True if equal, false otherwise.
 */
bool BindUnitTest::Check()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Setup storage for results. */
	m_result = new glw::GLubyte[s_texture_count_rgba];

	/* Setup pixel sotre modes.*/
	gl.pixelStorei(GL_UNPACK_ALIGNMENT, sizeof(glw::GLubyte));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPixelStorei has failed");

	gl.pixelStorei(GL_PACK_ALIGNMENT, sizeof(glw::GLubyte));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPixelStorei has failed");

	/* Query framebuffer's image. */
	gl.readPixels(0, 0, s_texture_width, s_texture_height, GL_RGBA, GL_UNSIGNED_BYTE, m_result);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays call has failed.");

	/* Compare values with reference. */
	for (glw::GLuint i = 0; i < s_texture_count_rgba; ++i)
	{
		if (s_texture_data_rgba[i] != m_result[i])
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Framebuffer data " << DataToString(s_texture_count_rgba, m_result)
				<< " does not match the reference values " << DataToString(s_texture_count_rgba, s_texture_data_rgba)
				<< "." << tcu::TestLog::EndMessage;

			return false;
		}
	}

	return true;
}

/** @brief Clean GL objects, test variables and GL errors.
 */
void BindUnitTest::CleanAll()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Release GL objects. */
	if (m_po)
	{
		gl.useProgram(0);

		gl.deleteProgram(m_po);

		m_po = 0;
	}

	if (m_to[0] || m_to[1] || m_to[2] || m_to[3])
	{
		gl.deleteTextures(4, m_to);

		m_to[0] = 0;
		m_to[1] = 0;
		m_to[2] = 0;
		m_to[3] = 0;
	}

	if (m_fbo)
	{
		gl.deleteFramebuffers(1, &m_fbo);

		m_fbo = 0;
	}

	if (m_rbo)
	{
		gl.deleteRenderbuffers(1, &m_rbo);

		m_rbo = 0;
	}

	/* Release heap. */
	if (DE_NULL != m_result)
	{
		delete[] m_result;
	}

	/* Erros clean-up. */
	while (GL_NO_ERROR != gl.getError())
		;
}

/** @brief Convert raw data into string for logging purposes.
 *
 *  @param [in] count      Count of the data.
 *  @param [in] data       Raw data.
 *
 *  @return String representation of data.
 */
std::string BindUnitTest::DataToString(glw::GLuint count, const glw::GLubyte data[])
{
	std::string data_str = "[";

	for (glw::GLuint i = 0; i < count; ++i)
	{
		std::stringstream int_sstream;

		int_sstream << unsigned(data[i]);

		data_str.append(int_sstream.str());

		if (i + 1 < count)
		{
			data_str.append(", ");
		}
		else
		{
			data_str.append("]");
		}
	}

	return data_str;
}

/** Reference data and parameters. */
const glw::GLubyte BindUnitTest::s_texture_data_r[]	= { 0, 4, 8, 12, 16, 20 };
const glw::GLubyte BindUnitTest::s_texture_data_g[]	= { 1, 5, 9, 13, 17, 21 };
const glw::GLubyte BindUnitTest::s_texture_data_b[]	= { 2, 6, 10, 14, 18, 22 };
const glw::GLubyte BindUnitTest::s_texture_data_a[]	= { 3, 7, 11, 15, 19, 23 };
const glw::GLubyte BindUnitTest::s_texture_data_rgba[] = { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11,
														   12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23 };
const glw::GLuint BindUnitTest::s_texture_width		 = 2;
const glw::GLuint BindUnitTest::s_texture_height	 = 3;
const glw::GLuint BindUnitTest::s_texture_count_rgba = sizeof(s_texture_data_rgba) / sizeof(s_texture_data_rgba[0]);

/* Vertex shader source code. */
const glw::GLchar* BindUnitTest::s_vertex_shader = "#version 450\n"
												   "\n"
												   "void main()\n"
												   "{\n"
												   "    switch(gl_VertexID)\n"
												   "    {\n"
												   "        case 0:\n"
												   "            gl_Position = vec4(-1.0, 1.0, 0.0, 1.0);\n"
												   "            break;\n"
												   "        case 1:\n"
												   "            gl_Position = vec4( 1.0, 1.0, 0.0, 1.0);\n"
												   "            break;\n"
												   "        case 2:\n"
												   "            gl_Position = vec4(-1.0,-1.0, 0.0, 1.0);\n"
												   "            break;\n"
												   "        case 3:\n"
												   "            gl_Position = vec4( 1.0,-1.0, 0.0, 1.0);\n"
												   "            break;\n"
												   "    }\n"
												   "}\n";

/* Fragment shader source program. */
const glw::GLchar* BindUnitTest::s_fragment_shader =
	"#version 450\n"
	"\n"
	"layout(pixel_center_integer) in vec4 gl_FragCoord;\n"
	"\n"
	"uniform sampler2D texture_input_r;\n"
	"uniform sampler2D texture_input_g;\n"
	"uniform sampler2D texture_input_b;\n"
	"uniform sampler2D texture_input_a;\n"
	"\n"
	"out     vec4      color_output;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    color_output = vec4(texelFetch(texture_input_r, ivec2(gl_FragCoord.xy), 0).r,\n"
	"                        texelFetch(texture_input_g, ivec2(gl_FragCoord.xy), 0).r,\n"
	"                        texelFetch(texture_input_b, ivec2(gl_FragCoord.xy), 0).r,\n"
	"                        texelFetch(texture_input_a, ivec2(gl_FragCoord.xy), 0).r);\n"
	"}\n";

const glw::GLchar* BindUnitTest::s_fragment_shader_samplers[4] = { "texture_input_r", "texture_input_g",
																   "texture_input_b", "texture_input_a" };

/******************************** Get Image Test Implementation   ********************************/

/** @brief Get Image Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
GetImageTest::GetImageTest(deqp::Context& context)
	: deqp::TestCase(context, "textures_get_image", "Textures Get Image Test")
{
	/* Intentionally left blank */
}

/** Reference data. */
const glw::GLubyte GetImageTest::s_texture_data[] = { 0x0,  0x0,  0x0,  0xff, 0x7f, 0x7f, 0x7f, 0xff, 0xc3, 0xc3, 0xc3,
													  0xff, 0xff, 0xff, 0xff, 0xff, 0x88, 0x0,  0x15, 0xff, 0xed, 0x1c,
													  0x24, 0xff, 0xff, 0x7f, 0x27, 0xff, 0xff, 0xf2, 0x0,  0xff, 0xc8,
													  0xbf, 0xe7, 0xff, 0x70, 0x92, 0xbe, 0xff, 0x99, 0xd9, 0xea, 0xff,
													  0xb5, 0xe6, 0x1d, 0xff, 0xa3, 0x49, 0xa4, 0xff, 0x3f, 0x48, 0xcc,
													  0xff, 0x0,  0xa2, 0xe8, 0xff, 0x22, 0xb1, 0x4c, 0xff };

/** Reference data (compressed). */
const glw::GLubyte GetImageTest::s_texture_data_compressed[] = { 0xa6, 0x39, 0x9, 0xf1, 0x88, 0x8b, 0x75, 0x85 };

/** Reference data parameters. */
const glw::GLuint GetImageTest::s_texture_width			  = 4;
const glw::GLuint GetImageTest::s_texture_height		  = 4;
const glw::GLuint GetImageTest::s_texture_size			  = sizeof(s_texture_data);
const glw::GLuint GetImageTest::s_texture_size_compressed = sizeof(s_texture_data_compressed);
const glw::GLuint GetImageTest::s_texture_count			  = s_texture_size / sizeof(s_texture_data[0]);
const glw::GLuint GetImageTest::s_texture_count_compressed =
	s_texture_size_compressed / sizeof(s_texture_data_compressed[0]);

/** @brief Get Image Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult GetImageTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	/* Objects. */
	glw::GLuint  texture									   = 0;
	glw::GLubyte result[s_texture_count]					   = {};
	glw::GLubyte result_compressed[s_texture_count_compressed] = {};

	try
	{
		/* Uncompressed case. */
		{
			/* Texture initiation. */
			gl.genTextures(1, &texture);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

			gl.bindTexture(GL_TEXTURE_2D, texture);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

			gl.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, s_texture_width, s_texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
						  s_texture_data);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage2D has failed");

			/* Quering image with tested function. */
			gl.getTextureImage(texture, 0, GL_RGBA, GL_UNSIGNED_BYTE, sizeof(result), result);

			/* Check for errors. */
			glw::GLenum error = GL_NO_ERROR;

			if (GL_NO_ERROR != (error = gl.getError()))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "GetTextureImage unexpectedly generated error "
					<< glu::getErrorStr(error) << ". Test fails." << tcu::TestLog::EndMessage;

				is_ok = false;
			}
			else
			{
				/* No error, so compare images. */
				for (glw::GLuint i = 0; i < s_texture_count; ++i)
				{
					if (s_texture_data[i] != result[i])
					{
						m_context.getTestContext().getLog() << tcu::TestLog::Message << "GetTextureImage returned "
															<< DataToString(s_texture_count, result) << ", but "
															<< DataToString(s_texture_count, s_texture_data)
															<< " was expected. Test fails." << tcu::TestLog::EndMessage;

						is_ok = false;

						break;
					}
				}
			}
		}

		/* Clean up texture .*/
		gl.deleteTextures(1, &texture);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

		texture = 0;

		/* Compressed case. */
		{
			/* Texture initiation. */
			gl.genTextures(1, &texture);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

			gl.bindTexture(GL_TEXTURE_2D, texture);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

			gl.compressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB8_ETC2, s_texture_width, s_texture_height, 0,
									s_texture_size_compressed, s_texture_data_compressed);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glCompressedTexImage2D has failed");

			/* Quering image with tested function. */
			gl.getCompressedTextureImage(texture, 0, s_texture_count_compressed * sizeof(result_compressed[0]),
										 result_compressed);

			/* Check for errors. */
			glw::GLenum error = GL_NO_ERROR;

			if (GL_NO_ERROR != (error = gl.getError()))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "GetCompressedTextureImage unexpectedly generated error "
					<< glu::getErrorStr(error) << ". Test fails." << tcu::TestLog::EndMessage;

				is_ok = false;
			}
			else
			{
				/* No error, so compare images. */
				for (glw::GLuint i = 0; i < s_texture_count_compressed; ++i)
				{
					if (s_texture_data_compressed[i] != result_compressed[i])
					{
						m_context.getTestContext().getLog()
							<< tcu::TestLog::Message << "GetCompressedTextureImage returned "
							<< DataToString(s_texture_count_compressed, result_compressed) << ", but "
							<< DataToString(s_texture_count_compressed, s_texture_data_compressed)
							<< " was expected. Test fails." << tcu::TestLog::EndMessage;

						is_ok = false;

						break;
					}
				}
			}
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	if (texture)
	{
		gl.deleteTextures(1, &texture);
	}

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** @brief Convert raw data into string for logging purposes.
 *
 *  @param [in] count      Count of the data.
 *  @param [in] data       Raw data.
 *
 *  @return String representation of data.
 */
std::string GetImageTest::DataToString(glw::GLuint count, const glw::GLubyte data[])
{
	std::string data_str = "[";

	for (glw::GLuint i = 0; i < count; ++i)
	{
		std::stringstream int_sstream;

		int_sstream << unsigned(data[i]);

		data_str.append(int_sstream.str());

		if (i + 1 < count)
		{
			data_str.append(", ");
		}
		else
		{
			data_str.append("]");
		}
	}

	return data_str;
}

/******************************** Get Level Parameter Test Implementation   ********************************/

/** @brief Get Level Parameter Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
GetLevelParameterTest::GetLevelParameterTest(deqp::Context& context)
	: deqp::TestCase(context, "textures_get_level_parameter", "Textures Get Level Parameter Test")
{
	/* Intentionally left blank */
}

/** Reference data. */
const glw::GLubyte GetLevelParameterTest::s_texture_data[] = {
	0x0,  0x0,  0x0,  0xff, 0x7f, 0x7f, 0x7f, 0xff, 0xc3, 0xc3, 0xc3, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x88, 0x0,  0x15, 0xff, 0xed, 0x1c, 0x24, 0xff, 0xff, 0x7f, 0x27, 0xff, 0xff, 0xf2, 0x0,  0xff,
	0xc8, 0xbf, 0xe7, 0xff, 0x70, 0x92, 0xbe, 0xff, 0x99, 0xd9, 0xea, 0xff, 0xb5, 0xe6, 0x1d, 0xff,
	0xa3, 0x49, 0xa4, 0xff, 0x3f, 0x48, 0xcc, 0xff, 0x0,  0xa2, 0xe8, 0xff, 0x22, 0xb1, 0x4c, 0xff,

	0x0,  0x0,  0x0,  0xff, 0x7f, 0x7f, 0x7f, 0xff, 0xc3, 0xc3, 0xc3, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x88, 0x0,  0x15, 0xff, 0xed, 0x1c, 0x24, 0xff, 0xff, 0x7f, 0x27, 0xff, 0xff, 0xf2, 0x0,  0xff,
	0xc8, 0xbf, 0xe7, 0xff, 0x70, 0x92, 0xbe, 0xff, 0x99, 0xd9, 0xea, 0xff, 0xb5, 0xe6, 0x1d, 0xff,
	0xa3, 0x49, 0xa4, 0xff, 0x3f, 0x48, 0xcc, 0xff, 0x0,  0xa2, 0xe8, 0xff, 0x22, 0xb1, 0x4c, 0xff,

	0x0,  0x0,  0x0,  0xff, 0x7f, 0x7f, 0x7f, 0xff, 0xc3, 0xc3, 0xc3, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x88, 0x0,  0x15, 0xff, 0xed, 0x1c, 0x24, 0xff, 0xff, 0x7f, 0x27, 0xff, 0xff, 0xf2, 0x0,  0xff,
	0xc8, 0xbf, 0xe7, 0xff, 0x70, 0x92, 0xbe, 0xff, 0x99, 0xd9, 0xea, 0xff, 0xb5, 0xe6, 0x1d, 0xff,
	0xa3, 0x49, 0xa4, 0xff, 0x3f, 0x48, 0xcc, 0xff, 0x0,  0xa2, 0xe8, 0xff, 0x22, 0xb1, 0x4c, 0xff,

	0x0,  0x0,  0x0,  0xff, 0x7f, 0x7f, 0x7f, 0xff, 0xc3, 0xc3, 0xc3, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x88, 0x0,  0x15, 0xff, 0xed, 0x1c, 0x24, 0xff, 0xff, 0x7f, 0x27, 0xff, 0xff, 0xf2, 0x0,  0xff,
	0xc8, 0xbf, 0xe7, 0xff, 0x70, 0x92, 0xbe, 0xff, 0x99, 0xd9, 0xea, 0xff, 0xb5, 0xe6, 0x1d, 0xff,
	0xa3, 0x49, 0xa4, 0xff, 0x3f, 0x48, 0xcc, 0xff, 0x0,  0xa2, 0xe8, 0xff, 0x22, 0xb1, 0x4c, 0xff
};

/** Reference data parameters. */
const glw::GLuint GetLevelParameterTest::s_texture_width  = 4;
const glw::GLuint GetLevelParameterTest::s_texture_height = 4;
const glw::GLuint GetLevelParameterTest::s_texture_depth  = 4;

/** @brief Get Level Parameter Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult GetLevelParameterTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	/* Objects. */
	glw::GLuint texture = 0;

	try
	{
		/* Texture initiation. */
		gl.genTextures(1, &texture);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

		gl.bindTexture(GL_TEXTURE_3D, texture);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

		gl.texImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, s_texture_width, s_texture_height, s_texture_depth, 0, GL_RGBA,
					  GL_UNSIGNED_BYTE, s_texture_data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage2D has failed");

		gl.texImage3D(GL_TEXTURE_3D, 1, GL_RGBA8, s_texture_width / 2, s_texture_height / 2, s_texture_depth / 2, 0,
					  GL_RGBA, GL_UNSIGNED_BYTE, s_texture_data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage2D has failed");

		static const glw::GLenum pnames[] = {
			GL_TEXTURE_WIDTH,	  GL_TEXTURE_HEIGHT,	 GL_TEXTURE_DEPTH,		 GL_TEXTURE_INTERNAL_FORMAT,
			GL_TEXTURE_RED_TYPE,   GL_TEXTURE_GREEN_TYPE, GL_TEXTURE_BLUE_TYPE,  GL_TEXTURE_ALPHA_TYPE,
			GL_TEXTURE_DEPTH_TYPE, GL_TEXTURE_RED_SIZE,   GL_TEXTURE_GREEN_SIZE, GL_TEXTURE_BLUE_SIZE,
			GL_TEXTURE_ALPHA_SIZE, GL_TEXTURE_DEPTH_SIZE, GL_TEXTURE_COMPRESSED
		};
		static const glw::GLuint pnames_count = sizeof(pnames) / sizeof(pnames[0]);

		/* Test GetTextureLevelParameteriv. */
		for (glw::GLuint i = 0; i < 2 /* levels */; ++i)
		{
			for (glw::GLuint j = 0; j < pnames_count; ++j)
			{
				glw::GLint result_legacy = 0;
				glw::GLint result_dsa	= 0;

				/* Quering reference value. */
				gl.getTexLevelParameteriv(GL_TEXTURE_3D, i, pnames[j], &result_legacy);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameteriv has failed");

				/* Quering using DSA function. */
				gl.getTextureLevelParameteriv(texture, i, pnames[j], &result_dsa);

				/* Check for errors. */
				glw::GLenum error = GL_NO_ERROR;

				if (GL_NO_ERROR != (error = gl.getError()))
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "GetTextureLevelParameteriv unexpectedly generated error "
						<< glu::getErrorStr(error) << ". Test fails." << tcu::TestLog::EndMessage;

					is_ok = false;
				}
				else
				{
					/* Compare values. */
					if (result_legacy != result_dsa)
					{
						m_context.getTestContext().getLog()
							<< tcu::TestLog::Message << "For parameter name "
							<< glu::getTextureLevelParameterStr(pnames[j]) << " GetTextureLevelParameteriv returned "
							<< result_dsa << ", but reference value (queried using GetTexLevelParameteriv) was "
							<< result_legacy << ". Test fails." << tcu::TestLog::EndMessage;

						is_ok = false;
					}
				}
			}
		}

		/* Test GetTextureLevelParameterfv. */
		for (glw::GLuint i = 0; i < 2 /* levels */; ++i)
		{
			for (glw::GLuint j = 0; j < pnames_count; ++j)
			{
				glw::GLfloat result_legacy = 0.f;
				glw::GLfloat result_dsa	= 0.f;

				/* Quering reference value. */
				gl.getTexLevelParameterfv(GL_TEXTURE_3D, i, pnames[j], &result_legacy);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameterfv has failed");

				/* Quering using DSA function. */
				gl.getTextureLevelParameterfv(texture, i, pnames[j], &result_dsa);

				/* Check for errors. */
				glw::GLenum error = GL_NO_ERROR;

				if (GL_NO_ERROR != (error = gl.getError()))
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "GetTextureLevelParameterfv unexpectedly generated error "
						<< glu::getErrorStr(error) << ". Test fails." << tcu::TestLog::EndMessage;

					is_ok = false;
				}
				else
				{
					/* Compare values. */
					if (de::abs(result_legacy - result_dsa) > 0.125 /* Precision. */)
					{
						m_context.getTestContext().getLog()
							<< tcu::TestLog::Message << "For parameter name "
							<< glu::getTextureLevelParameterStr(pnames[j]) << " GetTextureLevelParameterfv returned "
							<< result_dsa << ", but reference value (queried using GetTexLevelParameterfv) was "
							<< result_legacy << ". Test fails." << tcu::TestLog::EndMessage;

						is_ok = false;
					}
				}
			}
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	if (texture)
	{
		gl.deleteTextures(1, &texture);
	}

	while (GL_NO_ERROR != gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/*********************************** Errors Utility Class *****************************************************/

/** @brief Check for errors and log.
 *
 *  @param [in] context             Test's context.
 *  @param [in] expected_error      Expected error value.
 *  @param [in] function_name       Name of the function (to be logged).
 *  @param [in] log                 Log message.
 *
 *  @return True if error is equal to expected, false otherwise.
 */
bool ErrorsUtilities::CheckErrorAndLog(deqp::Context& context, glw::GLuint expected_error,
									   const glw::GLchar* function_name, const glw::GLchar* log)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = context.getRenderContext().getFunctions();

	/* Check error. */
	glw::GLenum error = GL_NO_ERROR;

	if (expected_error != (error = gl.getError()))
	{
		context.getTestContext().getLog() << tcu::TestLog::Message << function_name << " generated error "
										  << glu::getErrorStr(error) << " but, " << glu::getErrorStr(expected_error)
										  << " was expected if " << log << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/******************************** Creation Errors Test Implementation   ********************************/

/** @brief Creation Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
CreationErrorsTest::CreationErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "textures_creation_errors", "Texture Objects Creation Errors Test")
{
	/* Intentionally left blank. */
}

/** @brief Iterate Creation Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult CreationErrorsTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	/* Textures' objects */
	glw::GLuint texture = 0;

	try
	{
		/* Not a target test. */
		gl.createTextures(NotATarget(), 1, &texture);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glCreateTextures",
								  "target is not one of the allowable values.");

		if (texture)
		{
			gl.deleteTextures(1, &texture);

			texture = 0;
		}

		/* Negative number of textures. */
		gl.createTextures(GL_TEXTURE_2D, -1, &texture);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glCreateTextures", "n is negative.");
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	if (texture)
	{
		gl.deleteTextures(1, &texture);
	}

	/* Errors clean up. */
	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** @brief Function retruns enum which is not a texture target.
 */
glw::GLenum CreationErrorsTest::NotATarget()
{
	static const glw::GLenum texture_targets[] = { GL_TEXTURE_1D,
												   GL_TEXTURE_2D,
												   GL_TEXTURE_3D,
												   GL_TEXTURE_1D_ARRAY,
												   GL_TEXTURE_2D_ARRAY,
												   GL_TEXTURE_RECTANGLE,
												   GL_TEXTURE_CUBE_MAP,
												   GL_TEXTURE_CUBE_MAP_ARRAY,
												   GL_TEXTURE_BUFFER,
												   GL_TEXTURE_2D_MULTISAMPLE,
												   GL_TEXTURE_2D_MULTISAMPLE_ARRAY };

	glw::GLenum not_a_target = 0;
	bool		is_target	= true;

	while (is_target)
	{
		not_a_target++;

		is_target = false;

		for (glw::GLuint i = 0; i < sizeof(texture_targets) / sizeof(texture_targets[0]); ++i)
		{
			if (texture_targets[i] == not_a_target)
			{
				is_target = true;
				break;
			}
		}
	}

	return not_a_target;
}

/******************************** Texture Buffer Errors Test Implementation   ********************************/

/** @brief Texture Buffer Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
BufferErrorsTest::BufferErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "textures_buffer_errors", "Texture Buffer Errors Test")
{
	/* Intentionally left blank. */
}

/** @brief Iterate Texture Buffer Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult BufferErrorsTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	/* Textures' objects */
	glw::GLuint texture_buffer = 0;
	glw::GLuint texture_1D	 = 0;
	glw::GLuint buffer		   = 0;

	static const glw::GLubyte data[4]   = { 1, 2, 3, 4 };
	static const glw::GLuint  data_size = sizeof(data);

	try
	{
		/* Auxiliary objects setup. */
		gl.createTextures(GL_TEXTURE_BUFFER, 1, &texture_buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

		gl.createTextures(GL_TEXTURE_1D, 1, &texture_1D);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

		gl.createBuffers(1, &buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers has failed");

		gl.namedBufferData(buffer, data_size, data, GL_STATIC_COPY);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBufferData has failed");

		/*  Check that INVALID_OPERATION is generated by glTextureBuffer if texture
		 is not the name of an existing texture object. */
		{
			glw::GLuint not_a_texture = 0;

			while (gl.isTexture(++not_a_texture))
				;
			GLU_EXPECT_NO_ERROR(gl.getError(), "glIsTexture has failed");

			gl.textureBuffer(not_a_texture, GL_RGBA8, buffer);

			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureBuffer",
									  "texture is not the name of an existing texture object.");
		}

		/*  Check that INVALID_ENUM is generated by glTextureBuffer if the effective
		 target of texture is not TEXTURE_BUFFER. */
		{
			gl.textureBuffer(texture_1D, GL_RGBA8, buffer);

			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureBuffer",
									  "the effective target of texture is not TEXTURE_BUFFER.");
		}

		/*  Check that INVALID_ENUM is generated if internalformat is not one of the
		 sized internal formats described above. */
		{
			gl.textureBuffer(texture_buffer, GL_COMPRESSED_SIGNED_RED_RGTC1, buffer);

			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureBuffer",
									  "internalformat is not one of the sized internal formats described above..");
		}

		/*  Check that INVALID_OPERATION is generated if buffer is not zero and is
		 not the name of an existing buffer object. */
		{
			glw::GLuint not_a_buffer = 0;

			while (gl.isBuffer(++not_a_buffer))
				;
			GLU_EXPECT_NO_ERROR(gl.getError(), "glIsBuffer has failed");

			gl.textureBuffer(texture_buffer, GL_RGBA8, not_a_buffer);

			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureBuffer",
									  "buffer is not zero and is not the name of an existing buffer object.");
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	if (texture_1D)
	{
		gl.deleteTextures(1, &texture_1D);
	}

	if (texture_buffer)
	{
		gl.deleteTextures(1, &texture_buffer);
	}

	if (buffer)
	{
		gl.deleteBuffers(1, &buffer);
	}

	/* Errors clean up. */
	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/******************************** Texture Buffer Range Errors Test Implementation   ********************************/

/** @brief Texture Buffer Range Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
BufferRangeErrorsTest::BufferRangeErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "textures_buffer_range_errors", "Texture Buffer Range Errors Test")
{
	/* Intentionally left blank. */
}

/** @brief Iterate Texture Buffer Range Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult BufferRangeErrorsTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	/* Textures' objects */
	glw::GLuint texture_buffer = 0;
	glw::GLuint texture_1D	 = 0;
	glw::GLuint buffer		   = 0;

	static const glw::GLubyte data[4]   = { 1, 2, 3, 4 };
	static const glw::GLuint  data_size = sizeof(data);

	try
	{
		/* Auxiliary objects setup. */
		gl.createTextures(GL_TEXTURE_BUFFER, 1, &texture_buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

		gl.createTextures(GL_TEXTURE_1D, 1, &texture_1D);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

		gl.createBuffers(1, &buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers has failed");

		gl.namedBufferData(buffer, data_size, data, GL_STATIC_COPY);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBufferData has failed");

		/*  Check that INVALID_OPERATION is generated by TextureBufferRange if
		 texture is not the name of an existing texture object.*/
		{
			glw::GLuint not_a_texture = 0;

			while (gl.isTexture(++not_a_texture))
				;
			GLU_EXPECT_NO_ERROR(gl.getError(), "glIsTexture has failed");

			gl.textureBufferRange(not_a_texture, GL_RGBA8, buffer, 0, data_size);

			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureBufferRange",
									  "texture is not the name of an existing texture object.");
		}

		/*  Check that INVALID_ENUM is generated by TextureBufferRange if the
		 effective target of texture is not TEXTURE_BUFFER. */
		{
			gl.textureBufferRange(texture_1D, GL_RGBA8, buffer, 0, data_size);

			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureBufferRange",
									  "the effective target of texture is not TEXTURE_BUFFER.");
		}

		/*  Check that INVALID_ENUM is generated by TextureBufferRange if
		 internalformat is not one of the sized internal formats described above. */
		{
			gl.textureBufferRange(texture_buffer, GL_COMPRESSED_SIGNED_RED_RGTC1, buffer, 0, data_size);

			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureBufferRange",
									  "internalformat is not one of the supported sized internal formats.");
		}

		/*  Check that INVALID_OPERATION is generated by TextureBufferRange if
		 buffer is not zero and is not the name of an existing buffer object. */
		{
			glw::GLuint not_a_buffer = 0;

			while (gl.isBuffer(++not_a_buffer))
				;
			GLU_EXPECT_NO_ERROR(gl.getError(), "glIsBuffer has failed");

			gl.textureBufferRange(texture_buffer, GL_RGBA8, not_a_buffer, 0, data_size);

			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureBufferRange",
									  "buffer is not zero and is not the name of an existing buffer object.");
		}

		/* Check that INVALID_VALUE is generated by TextureBufferRange if offset
		 is negative, if size is less than or equal to zero, or if offset + size
		 is greater than the value of BUFFER_SIZE for buffer. */
		{
			gl.textureBufferRange(texture_buffer, GL_RGBA8, buffer, -1, data_size);

			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureBufferRange", "offset is negative.");

			gl.textureBufferRange(texture_buffer, GL_RGBA8, buffer, 0, 0);

			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureBufferRange", "size is zero.");

			gl.textureBufferRange(texture_buffer, GL_RGBA8, buffer, 0, -1);

			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureBufferRange", "size is negative.");

			gl.textureBufferRange(texture_buffer, GL_RGBA8, buffer, 0, data_size * 16);

			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureBufferRange",
									  "size is greater than the value of BUFFER_SIZE for buffer.");
		}

		/* Check that INVALID_VALUE is generated by TextureBufferRange if offset is
		 not an integer multiple of the value of TEXTURE_BUFFER_OFFSET_ALIGNMENT. */
		{
			glw::GLint gl_texture_buffer_offset_alignment = 0;

			gl.getIntegerv(GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT, &gl_texture_buffer_offset_alignment);

			/* If alignmet is 1 we cannot do anything. Error situtation is impossible then. */
			if (gl_texture_buffer_offset_alignment > 1)
			{
				gl.textureBufferRange(texture_buffer, GL_RGBA8, buffer, 1, data_size - 1);

				is_ok &= CheckErrorAndLog(
					m_context, GL_INVALID_VALUE, "glTextureBufferRange",
					"offset is not an integer multiple of the value of TEXTURE_BUFFER_OFFSET_ALIGNMENT.");
			}
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	if (texture_1D)
	{
		gl.deleteTextures(1, &texture_1D);
	}

	if (texture_buffer)
	{
		gl.deleteTextures(1, &texture_buffer);
	}

	if (buffer)
	{
		gl.deleteBuffers(1, &buffer);
	}

	/* Errors clean up. */
	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/******************************** Texture Storage Errors Test Implementation   ********************************/

/** @brief Texture Storage Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
StorageErrorsTest::StorageErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "textures_storage_errors", "Texture Storage Errors Test")
	, m_to_1D(0)
	, m_to_1D_array(0)
	, m_to_2D(0)
	, m_to_2D_array(0)
	, m_to_3D(0)
	, m_to_2D_ms(0)
	, m_to_2D_ms_immutable(0)
	, m_to_3D_ms(0)
	, m_to_3D_ms_immutable(0)
	, m_to_invalid(0)
	, m_internalformat_invalid(0)
	, m_max_texture_size(1)
	, m_max_samples(1)
	, m_max_array_texture_layers(1)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Texture Storage Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult StorageErrorsTest::iterate()
{
	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		Prepare();

		is_ok &= Test1D();
		is_ok &= Test2D();
		is_ok &= Test3D();
		is_ok &= Test2DMultisample();
		is_ok &= Test3DMultisample();
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	Clean();

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** @brief Prepare test objects.
 */
void StorageErrorsTest::Prepare()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Auxiliary objects setup. */

	/* 1D */
	gl.createTextures(GL_TEXTURE_1D, 1, &m_to_1D);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	/* 1D ARRAY */
	gl.createTextures(GL_TEXTURE_1D_ARRAY, 1, &m_to_1D_array);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	/* 2D */
	gl.createTextures(GL_TEXTURE_2D, 1, &m_to_2D);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	/* 2D ARRAY */
	gl.createTextures(GL_TEXTURE_2D_ARRAY, 1, &m_to_2D_array);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	/* 3D */
	gl.createTextures(GL_TEXTURE_3D, 1, &m_to_3D);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	/* 2D Multisample */
	gl.createTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &m_to_2D_ms);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	/* 2D Multisample with storage */
	gl.createTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &m_to_2D_ms_immutable);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	gl.textureStorage2DMultisample(m_to_2D_ms_immutable, 1, GL_R8, 16, 16, false);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTextureStorage2DMultisample has failed");

	/* 3D Multisample */
	gl.createTextures(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 1, &m_to_3D_ms);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	/* 3D Multisample with storage */
	gl.createTextures(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 1, &m_to_3D_ms_immutable);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	gl.textureStorage3DMultisample(m_to_3D_ms_immutable, 1, GL_R8, 16, 16, 16, false);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTextureStorage2DMultisample has failed");

	/* Invalid values */

	/* invalid texture object */
	while (gl.isTexture(++m_to_invalid))
		;
	GLU_EXPECT_NO_ERROR(gl.getError(), "glIsTexture has failed");

	/* invalid internal format */
	static const glw::GLenum all_internal_formats[] = { GL_R8,
														GL_R8_SNORM,
														GL_R16,
														GL_R16_SNORM,
														GL_RG8,
														GL_RG8_SNORM,
														GL_RG16,
														GL_RG16_SNORM,
														GL_R3_G3_B2,
														GL_RGB4,
														GL_RGB5,
														GL_RGB565,
														GL_RGB8,
														GL_RGB8_SNORM,
														GL_RGB10,
														GL_RGB12,
														GL_RGB16,
														GL_RGB16_SNORM,
														GL_RGBA2,
														GL_RGBA4,
														GL_RGB5_A1,
														GL_RGBA8,
														GL_RGBA8_SNORM,
														GL_RGB10_A2,
														GL_RGB10_A2UI,
														GL_RGBA12,
														GL_RGBA16,
														GL_RGBA16_SNORM,
														GL_SRGB8,
														GL_SRGB8_ALPHA8,
														GL_R16F,
														GL_RG16F,
														GL_RGB16F,
														GL_RGBA16F,
														GL_R32F,
														GL_RG32F,
														GL_RGB32F,
														GL_RGBA32F,
														GL_R11F_G11F_B10F,
														GL_RGB9_E5,
														GL_R8I,
														GL_R8UI,
														GL_R16I,
														GL_R16UI,
														GL_R32I,
														GL_R32UI,
														GL_RG8I,
														GL_RG8UI,
														GL_RG16I,
														GL_RG16UI,
														GL_RG32I,
														GL_RG32UI,
														GL_RGB8I,
														GL_RGB8UI,
														GL_RGB16I,
														GL_RGB16UI,
														GL_RGB32I,
														GL_RGB32UI,
														GL_RGBA8I,
														GL_RGBA8UI,
														GL_RGBA16I,
														GL_RGBA16UI,
														GL_RGBA32I,
														GL_RGBA32UI,
														GL_COMPRESSED_RED,
														GL_COMPRESSED_RG,
														GL_COMPRESSED_RGB,
														GL_COMPRESSED_RGBA,
														GL_COMPRESSED_SRGB,
														GL_COMPRESSED_SRGB_ALPHA,
														GL_COMPRESSED_RED_RGTC1,
														GL_COMPRESSED_SIGNED_RED_RGTC1,
														GL_COMPRESSED_RG_RGTC2,
														GL_COMPRESSED_SIGNED_RG_RGTC2,
														GL_COMPRESSED_RGBA_BPTC_UNORM,
														GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM,
														GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT,
														GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT,
														GL_COMPRESSED_RGB8_ETC2,
														GL_COMPRESSED_SRGB8_ETC2,
														GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,
														GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2,
														GL_COMPRESSED_RGBA8_ETC2_EAC,
														GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,
														GL_COMPRESSED_R11_EAC,
														GL_COMPRESSED_SIGNED_R11_EAC,
														GL_COMPRESSED_RG11_EAC,
														GL_COMPRESSED_SIGNED_RG11_EAC,
														GL_DEPTH_COMPONENT16,
														GL_DEPTH_COMPONENT24,
														GL_DEPTH_COMPONENT32,
														GL_DEPTH_COMPONENT32F,
														GL_DEPTH24_STENCIL8,
														GL_DEPTH32F_STENCIL8,
														GL_STENCIL_INDEX1,
														GL_STENCIL_INDEX4,
														GL_STENCIL_INDEX8,
														GL_STENCIL_INDEX16 };

	static const glw::GLuint all_internal_formats_count =
		sizeof(all_internal_formats) / sizeof(all_internal_formats[0]);

	bool is_valid			 = true;
	m_internalformat_invalid = 0;

	while (is_valid)
	{
		is_valid = false;
		m_internalformat_invalid++;
		for (glw::GLuint i = 0; i < all_internal_formats_count; ++i)
		{
			if (all_internal_formats[i] == m_internalformat_invalid)
			{
				is_valid = true;
				break;
			}
		}
	}

	/* Maximum texture size.*/
	gl.getIntegerv(GL_MAX_TEXTURE_SIZE, &m_max_texture_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv has failed");

	/* Maximum number of samples. */
	gl.getIntegerv(GL_MAX_SAMPLES, &m_max_samples);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv has failed");

	/* Maximum number of array texture layers. */
	gl.getIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &m_max_array_texture_layers);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv has failed");
}

/** @brief Test TextureStorage1D
 *
 *  @return Test result.
 */
bool StorageErrorsTest::Test1D()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Result. */
	bool is_ok = true;

	/*  Check that INVALID_OPERATION is generated by TextureStorage1D if texture
	 is not the name of an existing texture object. */
	{
		gl.textureStorage1D(m_to_invalid, 1, GL_R8, 8);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureStorage1D",
								  "texture is not the name of an existing texture object.");
	}

	/*  Check that INVALID_ENUM is generated by TextureStorage1D if
	 internalformat is not a valid sized internal format. */
	{
		gl.textureStorage1D(m_to_1D, 1, m_internalformat_invalid, 8);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureStorage1D",
								  "internalformat is not a valid sized internal format.");
	}

	/*  Check that INVALID_ENUM is generated by TextureStorage1D if target or
	 the effective target of texture is not one of the accepted targets
	 described above. */
	{
		gl.textureStorage1D(m_to_2D, 1, GL_R8, 8);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureStorage1D",
								  "the effective target of texture is not one of the accepted targets.");
	}

	/*  Check that INVALID_VALUE is generated by TextureStorage1D if width or
	 levels are less than 1. */
	{
		gl.textureStorage1D(m_to_1D, 0, GL_R8, 8);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureStorage1D", "levels is less than 1.");

		gl.textureStorage1D(m_to_1D, 1, GL_R8, 0);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureStorage1D", "width is less than 1.");
	}

	/*  Check that INVALID_OPERATION is generated by TextureStorage1D if levels
	 is greater than log2(width)+1. */
	{
		gl.textureStorage1D(m_to_1D, 8, GL_R8, 8);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureStorage1D",
								  "levels is greater than log2(width)+1.");
	}

	return is_ok;
}

/** @brief Test TextureStorage2D
 *
 *  @return Test result.
 */
bool StorageErrorsTest::Test2D()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Result. */
	bool is_ok = true;

	/*  Check that INVALID_OPERATION is generated by TextureStorage2D if
	 texture is not the name of an existing texture object. */
	{
		gl.textureStorage2D(m_to_invalid, 1, GL_R8, 8, 8);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureStorage2D",
								  "texture is not the name of an existing texture object.");
	}

	/*  Check that INVALID_ENUM is generated by TextureStorage2D if
	 internalformat is not a valid sized internal format. */
	{
		gl.textureStorage2D(m_to_2D, 1, m_internalformat_invalid, 8, 8);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureStorage2D",
								  "internalformat is not a valid sized internal format.");
	}

	/*  Check that INVALID_ENUM is generated by TextureStorage2D if target or
	 the effective target of texture is not one of the accepted targets
	 described above. */
	{
		gl.textureStorage2D(m_to_1D, 1, GL_R8, 8, 8);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureStorage2D",
								  "the effective target of texture is not one of the accepted targets.");
	}

	/*  Check that INVALID_VALUE is generated by TextureStorage2D if width,
	 height or levels are less than 1. */
	{
		gl.textureStorage2D(m_to_2D, 0, GL_R8, 8, 8);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureStorage2D", "levels is less than 1.");

		gl.textureStorage2D(m_to_2D, 1, GL_R8, 0, 8);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureStorage2D", "width is less than 1.");

		gl.textureStorage2D(m_to_2D, 1, GL_R8, 8, 0);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureStorage2D", "height is less than 1.");
	}

	/* Check that INVALID_OPERATION is generated by TextureStorage2D if target
	 is TEXTURE_1D_ARRAY or PROXY_TEXTURE_1D_ARRAY and levels is greater than
	 log2(width)+1. */
	{
		gl.textureStorage2D(m_to_1D_array, 8, GL_R8, 8, 8);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureStorage2D",
								  "target is TEXTURE_1D_ARRAY and levels is greater than log2(width)+1.");
	}

	/*  Check that INVALID_OPERATION is generated by TextureStorage2D if target
	 is not TEXTURE_1D_ARRAY or PROXY_TEXTURE_1D_ARRAY and levels is greater
	 than log2(max(width, height))+1.  */
	{
		gl.textureStorage2D(m_to_2D, 8, GL_R8, 8, 8);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureStorage2D",
								  "target is TEXTURE_2D and levels is greater than log2(max(width, height))+1.");
	}

	return is_ok;
}

/** @brief Test TextureStorage3D
 *
 *  @return Test result.
 */
bool StorageErrorsTest::Test3D()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Result. */
	bool is_ok = true;

	/*  Check that INVALID_OPERATION is generated by TextureStorage3D if texture
	 is not the name of an existing texture object. */
	{
		gl.textureStorage3D(m_to_invalid, 1, GL_R8, 8, 8, 8);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureStorage3D",
								  "texture is not the name of an existing texture object.");
	}

	/*  Check that INVALID_ENUM is generated by TextureStorage3D if
	 internalformat is not a valid sized internal format. */
	{
		gl.textureStorage3D(m_to_3D, 1, m_internalformat_invalid, 8, 8, 8);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureStorage3D",
								  "internalformat is not a valid sized internal format.");
	}

	/*  Check that INVALID_ENUM is generated by TextureStorage3D if target or
	 the effective target of texture is not one of the accepted targets
	 described above. */
	{
		gl.textureStorage3D(m_to_1D, 1, GL_R8, 8, 8, 8);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureStorage3D",
								  "the effective target of texture is not one of the accepted targets.");
	}

	/*  Check that INVALID_VALUE is generated by TextureStorage3D if width,
	 height, depth or levels are less than 1. */
	{
		gl.textureStorage3D(m_to_3D, 0, GL_R8, 8, 8, 8);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureStorage3D", "levels is less than 1.");

		gl.textureStorage3D(m_to_3D, 1, GL_R8, 0, 8, 8);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureStorage3D", "width is less than 1.");

		gl.textureStorage3D(m_to_3D, 1, GL_R8, 8, 0, 8);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureStorage3D", "height is less than 1.");

		gl.textureStorage3D(m_to_3D, 1, GL_R8, 8, 8, 0);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureStorage3D", "depth is less than 1.");
	}

	/* Check that INVALID_OPERATION is generated by TextureStorage3D if target
	 is TEXTURE_3D or PROXY_TEXTURE_3D and levels is greater than
	 log2(max(width, height, depth))+1. */
	{
		gl.textureStorage3D(m_to_3D, 8, GL_R8, 8, 8, 8);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureStorage3D",
								  "target is TEXTURE_3D and levels is greater than log2(max(width, height, depth))+1.");
	}

	/*  Check that INVALID_OPERATION is generated by TextureStorage3D if target
	 is TEXTURE_2D_ARRAY, PROXY_TEXTURE_2D_ARRAY, TEXURE_CUBE_ARRAY,
	 or PROXY_TEXTURE_CUBE_MAP_ARRAY and levels is greater than
	 log2(max(width, height))+1.  */
	{
		gl.textureStorage3D(m_to_2D_array, 6, GL_R8, 8, 8, 256);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureStorage3D",
								  "target is TEXTURE_2D_ARRAY and levels is greater than log2(max(width, height))+1.");
	}

	return is_ok;
}

/** @brief Test TextureStorage2DMultisample
 *
 *  @return Test result.
 */
bool StorageErrorsTest::Test2DMultisample()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Result. */
	bool is_ok = true;

	/*  Check that INVALID_OPERATION is generated by TextureStorage2DMultisample
	 if texture is not the name of an existing texture object. */
	{
		gl.textureStorage2DMultisample(m_to_invalid, 1, GL_R8, 8, 8, false);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureStorage2DMultisample",
								  "texture is not the name of an existing texture object.");
	}

	/*  Check that INVALID_ENUM is generated by TextureStorage2DMultisample if
	 internalformat is not a valid color-renderable, depth-renderable or
	 stencil-renderable format. */
	{
		gl.textureStorage2DMultisample(m_to_2D_ms, 1, m_internalformat_invalid, 8, 8, false);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureStorage2DMultisample",
								  "internalformat is not a valid sized internal format.");
	}

	/*  Check that INVALID_OPERATION is generated by TextureStorage2DMultisample if
	 target or the effective target of texture is not one of the accepted
	 targets described above. */
	{
		gl.textureStorage2DMultisample(m_to_1D, 1, GL_R8, 8, 8, false);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureStorage2DMultisample",
								  "the effective target of texture is not one of the accepted targets.");
	}

	/* Check that INVALID_VALUE is generated by TextureStorage2DMultisample if
	 width or height are less than 1 or greater than the value of
	 MAX_TEXTURE_SIZE. */
	{
		gl.textureStorage2DMultisample(m_to_2D_ms, 1, GL_R8, 0, 8, false);
		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureStorage2DMultisample", "width is less than 1.");

		gl.textureStorage2DMultisample(m_to_2D_ms, 1, GL_R8, 8, 0, false);
		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureStorage2DMultisample", "height is less than 1.");

		gl.textureStorage2DMultisample(m_to_2D_ms, 1, GL_R8, m_max_texture_size * 2, 8, false);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureStorage2DMultisample",
								  "width is greater than the value of MAX_TEXTURE_SIZE.");

		gl.textureStorage2DMultisample(m_to_2D_ms, 1, GL_R8, 8, m_max_texture_size * 2, false);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureStorage2DMultisample",
								  "height is greater than the value of MAX_TEXTURE_SIZE.");
	}

	/* Check that INVALID_OPERATION is generated by TextureStorage2DMultisample if
	 samples is greater than the value of MAX_SAMPLES. */
	{
		gl.textureStorage2DMultisample(m_to_2D_ms, m_max_samples * 2, GL_R8, 8, 8, false);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureStorage2DMultisample",
								  "samples is greater than the value of MAX_SAMPLES.");
	}

	/* Check that INVALID_OPERATION is generated by TextureStorage2DMultisample
	 if the value of TEXTURE_IMMUTABLE_FORMAT for the texture bound to target
	 is not FALSE. */
	{
		gl.textureStorage2DMultisample(m_to_2D_ms_immutable, 1, GL_R8, 8, 8, false);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureStorage2DMultisample",
								  "samples is greater than the value of MAX_SAMPLES.");
	}

	return is_ok;
}

/** @brief Test TextureStorage3DMultisample
 *
 *  @return Test result.
 */
bool StorageErrorsTest::Test3DMultisample()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Result. */
	bool is_ok = true;

	/*  Check that INVALID_OPERATION is generated by TextureStorage3DMultisample
	 if texture is not the name of an existing texture object. */
	{
		gl.textureStorage3DMultisample(m_to_invalid, 1, GL_R8, 8, 8, 8, false);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureStorage3DMultisample",
								  "texture is not the name of an existing texture object.");
	}

	/*  Check that INVALID_ENUM is generated by TextureStorage3DMultisample if
	 internalformat is not a valid color-renderable, depth-renderable or
	 stencil-renderable format. */
	{
		gl.textureStorage3DMultisample(m_to_3D_ms, 1, m_internalformat_invalid, 8, 8, 8, false);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureStorage3DMultisample",
								  "internalformat is not a valid sized internal format.");
	}

	/*  Check that INVALID_OPERATION is generated by TextureStorage3DMultisample if
	 target or the effective target of texture is not one of the accepted
	 targets described above. */
	{
		gl.textureStorage3DMultisample(m_to_1D, 1, GL_R8, 8, 8, 8, false);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureStorage3DMultisample",
								  "the effective target of texture is not one of the accepted targets.");
	}

	/* Check that INVALID_VALUE is generated by TextureStorage3DMultisample if
	 width or height are less than 1 or greater than the value of
	 MAX_TEXTURE_SIZE. */
	{
		gl.textureStorage3DMultisample(m_to_3D_ms, 1, GL_R8, 0, 8, 8, false);
		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureStorage3DMultisample", "width is less than 1.");

		gl.textureStorage3DMultisample(m_to_3D_ms, 1, GL_R8, 8, 0, 8, false);
		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureStorage3DMultisample", "height is less than 1.");

		gl.textureStorage3DMultisample(m_to_3D_ms, 1, GL_R8, m_max_texture_size * 2, 8, 8, false);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureStorage3DMultisample",
								  "width is greater than the value of MAX_TEXTURE_SIZE.");

		gl.textureStorage3DMultisample(m_to_3D_ms, 1, GL_R8, 8, m_max_texture_size * 2, 8, false);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureStorage3DMultisample",
								  "height is greater than the value of MAX_TEXTURE_SIZE.");
	}

	/* Check that INVALID_VALUE is generated by TextureStorage3DMultisample if
	 depth is less than 1 or greater than the value of
	 MAX_ARRAY_TEXTURE_LAYERS. */
	{
		gl.textureStorage3DMultisample(m_to_3D_ms, 1, GL_R8, 8, 8, 0, false);
		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureStorage3DMultisample", "depth is less than 1.");

		gl.textureStorage3DMultisample(m_to_3D_ms, 1, GL_R8, 8, 8, m_max_array_texture_layers * 2, false);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureStorage3DMultisample",
								  "depth is greater than the value of MAX_ARRAY_TEXTURE_LAYERS.");
	}

	/* Check that INVALID_VALUE is generated by TextureStorage3DMultisample if
	 samples is greater than the value of MAX_SAMPLES. */
	{
		gl.textureStorage3DMultisample(m_to_3D_ms, m_max_samples * 2, GL_R8, 8, 8, 8, false);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureStorage3DMultisample",
								  "samples is greater than the value of MAX_SAMPLES.");
	}

	/* Check that INVALID_OPERATION is generated by TextureStorage3DMultisample
	 if the value of TEXTURE_IMMUTABLE_FORMAT for the texture bound to target
	 is not FALSE. */
	{
		gl.textureStorage3DMultisample(m_to_3D_ms_immutable, 1, GL_R8, 8, 8, 8, false);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureStorage3DMultisample",
								  "samples is greater than the value of MAX_SAMPLES.");
	}

	return is_ok;
}

/** @brief Clean GL objects, test variables and GL errors.
 */
void StorageErrorsTest::Clean()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Cleanup. */
	if (m_to_1D)
	{
		gl.deleteTextures(1, &m_to_1D);

		m_to_1D = 0;
	}

	if (m_to_1D_array)
	{
		gl.deleteTextures(1, &m_to_1D_array);

		m_to_1D_array = 0;
	}

	if (m_to_2D)
	{
		gl.deleteTextures(1, &m_to_2D);

		m_to_2D = 0;
	}

	if (m_to_2D_array)
	{
		gl.deleteTextures(1, &m_to_2D_array);

		m_to_2D_array = 0;
	}

	if (m_to_3D)
	{
		gl.deleteTextures(1, &m_to_3D);

		m_to_3D = 0;
	}

	if (m_to_2D_ms)
	{
		gl.deleteTextures(1, &m_to_2D_ms);

		m_to_2D_ms = 0;
	}

	if (m_to_2D_ms_immutable)
	{
		gl.deleteTextures(1, &m_to_2D_ms_immutable);

		m_to_2D_ms_immutable = 0;
	}

	if (m_to_3D_ms)
	{
		gl.deleteTextures(1, &m_to_3D_ms);

		m_to_3D_ms = 0;
	}

	if (m_to_3D_ms_immutable)
	{
		gl.deleteTextures(1, &m_to_3D_ms_immutable);

		m_to_3D_ms_immutable = 0;
	}

	m_to_invalid			   = 0;
	m_internalformat_invalid   = 0;
	m_max_texture_size		   = 1;
	m_max_samples			   = 1;
	m_max_array_texture_layers = 1;

	while (GL_NO_ERROR != gl.getError())
		;
}

/******************************** Texture SubImage Errors Test Implementation   ********************************/

/** @brief Texture SubImage Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
SubImageErrorsTest::SubImageErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "textures_subimage_errors", "Texture SubImage Errors Test")
	, m_to_1D_empty(0)
	, m_to_2D_empty(0)
	, m_to_3D_empty(0)
	, m_to_1D(0)
	, m_to_2D(0)
	, m_to_3D(0)
	, m_to_1D_compressed(0)
	, m_to_2D_compressed(0)
	, m_to_3D_compressed(0)
	, m_to_rectangle_compressed(0)
	, m_to_invalid(0)
	, m_bo(0)
	, m_format_invalid(0)
	, m_type_invalid(0)
	, m_max_texture_size(1)
	, m_reference_compressed_1D(DE_NULL)
	, m_reference_compressed_2D(DE_NULL)
	, m_reference_compressed_3D(DE_NULL)
	, m_reference_compressed_rectangle(DE_NULL)
	, m_reference_compressed_1D_size(0)
	, m_reference_compressed_2D_size(0)
	, m_reference_compressed_3D_size(0)
	, m_reference_compressed_rectangle_size(0)
	, m_reference_compressed_1D_format(0)
	, m_reference_compressed_2D_format(0)
	, m_reference_compressed_3D_format(0)
	, m_reference_compressed_rectangle_format(0)
	, m_not_matching_compressed_1D_format(0)
	, m_not_matching_compressed_1D_size(0)
	, m_not_matching_compressed_2D_format(0)
	, m_not_matching_compressed_2D_size(0)
	, m_not_matching_compressed_3D_format(0)
	, m_not_matching_compressed_3D_size(0)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Texture SubImage Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult SubImageErrorsTest::iterate()
{
	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		Prepare();

		is_ok &= Test1D();
		is_ok &= Test2D();
		is_ok &= Test3D();
		is_ok &= Test1DCompressed();
		is_ok &= Test2DCompressed();
		is_ok &= Test3DCompressed();
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	Clean();

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** @brief Prepare test's objects.
 */
void SubImageErrorsTest::Prepare()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Auxiliary objects setup. */

	/* 1D */
	gl.createTextures(GL_TEXTURE_1D, 1, &m_to_1D_empty);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	/* 2D */
	gl.createTextures(GL_TEXTURE_2D, 1, &m_to_2D_empty);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	/* 3D */
	gl.createTextures(GL_TEXTURE_3D, 1, &m_to_3D_empty);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	/* 1D */
	gl.createTextures(GL_TEXTURE_1D, 1, &m_to_1D);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	gl.bindTexture(GL_TEXTURE_1D, m_to_1D);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

	gl.texImage1D(GL_TEXTURE_1D, 0, s_reference_internalformat, s_reference_width, 0, s_reference_format,
				  GL_UNSIGNED_BYTE, s_reference);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage1D has failed");

	/* 2D */
	gl.createTextures(GL_TEXTURE_2D, 1, &m_to_2D);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	gl.bindTexture(GL_TEXTURE_2D, m_to_2D);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

	gl.texImage2D(GL_TEXTURE_2D, 0, s_reference_internalformat, s_reference_width, s_reference_height, 0,
				  s_reference_format, GL_UNSIGNED_BYTE, s_reference);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage1D has failed");

	/* 3D */
	gl.createTextures(GL_TEXTURE_3D, 1, &m_to_3D);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	gl.bindTexture(GL_TEXTURE_3D, m_to_3D);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

	gl.texImage3D(GL_TEXTURE_3D, 0, s_reference_internalformat, s_reference_width, s_reference_height,
				  s_reference_depth, 0, s_reference_format, GL_UNSIGNED_BYTE, s_reference);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage1D has failed");

	/* 1D Compressed */
	gl.createTextures(GL_TEXTURE_1D, 1, &m_to_1D_compressed);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	gl.bindTexture(GL_TEXTURE_1D, m_to_1D_compressed);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

	gl.texImage1D(GL_TEXTURE_1D, 0, s_reference_internalformat_compressed, s_reference_width, 0, s_reference_format,
				  GL_UNSIGNED_BYTE, s_reference);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage1D has failed");

	glw::GLint is_compressed = 0;

	gl.getTexLevelParameteriv(GL_TEXTURE_1D, 0, GL_TEXTURE_COMPRESSED, &is_compressed);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTetTexLevelParameteriv has failed");

	if (is_compressed)
	{
		gl.getTexLevelParameteriv(GL_TEXTURE_1D, 0, GL_TEXTURE_INTERNAL_FORMAT, &m_reference_compressed_1D_format);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameteriv has failed");

		m_reference_compressed_1D_size = 0;

		gl.getTexLevelParameteriv(GL_TEXTURE_1D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &m_reference_compressed_1D_size);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameteriv has failed");

		if (m_reference_compressed_1D_size)
		{
			m_reference_compressed_1D = new glw::GLubyte[m_reference_compressed_1D_size];

			gl.getCompressedTexImage(GL_TEXTURE_1D, 0, m_reference_compressed_1D);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetCompressedTexImage has failed");
		}
	}

	/* 2D Compressed */
	gl.createTextures(GL_TEXTURE_2D, 1, &m_to_2D_compressed);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	gl.bindTexture(GL_TEXTURE_2D, m_to_2D_compressed);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

	gl.texImage2D(GL_TEXTURE_2D, 0, s_reference_internalformat_compressed, s_reference_width, s_reference_height, 0,
				  s_reference_format, GL_UNSIGNED_BYTE, s_reference);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage2D has failed");

	is_compressed = 0;

	gl.getTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED, &is_compressed);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTetTexLevelParameteriv has failed");

	if (is_compressed)
	{
		gl.getTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &m_reference_compressed_2D_format);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameteriv has failed");

		m_reference_compressed_2D_size = 0;

		gl.getTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &m_reference_compressed_2D_size);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameteriv has failed");

		if (m_reference_compressed_2D_size)
		{
			m_reference_compressed_2D = new glw::GLubyte[m_reference_compressed_2D_size];

			gl.getCompressedTexImage(GL_TEXTURE_2D, 0, m_reference_compressed_2D);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetCompressedTexImage has failed");
		}
	}

	/* 3D Compressed */
	gl.createTextures(GL_TEXTURE_2D_ARRAY, 1, &m_to_3D_compressed);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	gl.bindTexture(GL_TEXTURE_2D_ARRAY, m_to_3D_compressed);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

	gl.texImage3D(GL_TEXTURE_2D_ARRAY, 0, s_reference_internalformat_compressed, s_reference_width, s_reference_height,
				  s_reference_depth, 0, s_reference_format, GL_UNSIGNED_BYTE, s_reference);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage3D has failed");

	is_compressed = 0;

	gl.getTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, 0, GL_TEXTURE_COMPRESSED, &is_compressed);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTetTexLevelParameteriv has failed");

	if (is_compressed)
	{
		gl.getTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, 0, GL_TEXTURE_INTERNAL_FORMAT,
								  &m_reference_compressed_3D_format);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameteriv has failed");

		m_reference_compressed_3D_size = 0;

		gl.getTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE,
								  &m_reference_compressed_3D_size);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameteriv has failed");

		if (m_reference_compressed_3D_size)
		{
			m_reference_compressed_3D = new glw::GLubyte[m_reference_compressed_3D_size];

			gl.getCompressedTexImage(GL_TEXTURE_2D_ARRAY, 0, m_reference_compressed_3D);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetCompressedTexImage has failed");
		}
	}

	/* RECTANGLE Compressed */
	gl.createTextures(GL_TEXTURE_RECTANGLE, 1, &m_to_rectangle_compressed);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	gl.bindTexture(GL_TEXTURE_RECTANGLE, m_to_rectangle_compressed);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

	gl.texImage2D(GL_TEXTURE_RECTANGLE, 0, s_reference_internalformat_compressed, s_reference_width, s_reference_height,
				  0, s_reference_format, GL_UNSIGNED_BYTE, s_reference);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage2D has failed");

	is_compressed = 0;

	gl.getTexLevelParameteriv(GL_TEXTURE_RECTANGLE, 0, GL_TEXTURE_COMPRESSED, &is_compressed);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTetTexLevelParameteriv has failed");

	if (is_compressed)
	{
		gl.getTexLevelParameteriv(GL_TEXTURE_RECTANGLE, 0, GL_TEXTURE_INTERNAL_FORMAT,
								  &m_reference_compressed_rectangle_format);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameteriv has failed");

		m_reference_compressed_rectangle_size = 0;

		gl.getTexLevelParameteriv(GL_TEXTURE_RECTANGLE, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE,
								  &m_reference_compressed_rectangle_size);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameteriv has failed");

		if (m_reference_compressed_rectangle_size)
		{
			m_reference_compressed_rectangle = new glw::GLubyte[m_reference_compressed_rectangle_size];

			gl.getCompressedTexImage(GL_TEXTURE_RECTANGLE, 0, m_reference_compressed_rectangle);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetCompressedTexImage has failed");
		}
	}

	/* Buffer object */
	gl.createBuffers(1, &m_bo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers has failed");

	gl.namedBufferData(m_bo, s_reference_size, s_reference, GL_STATIC_COPY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBufferData has failed");

	/* Invalid values */

	/* invalid texture object */
	while (gl.isTexture(++m_to_invalid))
		;
	GLU_EXPECT_NO_ERROR(gl.getError(), "glIsTexture has failed");

	/* invalid internal format */
	static const glw::GLenum all_formats[] = { GL_STENCIL_INDEX,
											   GL_DEPTH_COMPONENT,
											   GL_DEPTH_STENCIL,
											   GL_RED,
											   GL_GREEN,
											   GL_BLUE,
											   GL_RG,
											   GL_RGB,
											   GL_RGBA,
											   GL_BGR,
											   GL_BGRA,
											   GL_RED_INTEGER,
											   GL_GREEN_INTEGER,
											   GL_BLUE_INTEGER,
											   GL_RG_INTEGER,
											   GL_RGB_INTEGER,
											   GL_RGBA_INTEGER,
											   GL_BGR_INTEGER,
											   GL_BGRA_INTEGER };

	static const glw::GLuint all_internal_formats_count = sizeof(all_formats) / sizeof(all_formats[0]);

	bool is_valid	= true;
	m_format_invalid = 0;

	while (is_valid)
	{
		is_valid = false;
		m_format_invalid++;
		for (glw::GLuint i = 0; i < all_internal_formats_count; ++i)
		{
			if (all_formats[i] == m_format_invalid)
			{
				is_valid = true;
				break;
			}
		}
	}

	/* Invalid type. */
	static const glw::GLenum all_types[] = { GL_UNSIGNED_BYTE,
											 GL_BYTE,
											 GL_UNSIGNED_SHORT,
											 GL_SHORT,
											 GL_UNSIGNED_INT,
											 GL_INT,
											 GL_HALF_FLOAT,
											 GL_FLOAT,
											 GL_UNSIGNED_BYTE_3_3_2,
											 GL_UNSIGNED_BYTE_2_3_3_REV,
											 GL_UNSIGNED_SHORT_5_6_5,
											 GL_UNSIGNED_SHORT_5_6_5_REV,
											 GL_UNSIGNED_SHORT_4_4_4_4,
											 GL_UNSIGNED_SHORT_4_4_4_4_REV,
											 GL_UNSIGNED_SHORT_5_5_5_1,
											 GL_UNSIGNED_SHORT_1_5_5_5_REV,
											 GL_UNSIGNED_INT_8_8_8_8,
											 GL_UNSIGNED_INT_8_8_8_8_REV,
											 GL_UNSIGNED_INT_10_10_10_2,
											 GL_UNSIGNED_INT_2_10_10_10_REV,
											 GL_UNSIGNED_INT_24_8,
											 GL_UNSIGNED_INT_10F_11F_11F_REV,
											 GL_UNSIGNED_INT_5_9_9_9_REV,
											 GL_FLOAT_32_UNSIGNED_INT_24_8_REV };

	static const glw::GLuint all_types_count = sizeof(all_types) / sizeof(all_types[0]);

	is_valid	   = true;
	m_type_invalid = 0;

	while (is_valid)
	{
		is_valid = false;
		m_type_invalid++;
		for (glw::GLuint i = 0; i < all_types_count; ++i)
		{
			if (all_types[i] == m_type_invalid)
			{
				is_valid = true;
				break;
			}
		}
	}

	/* Maximum texture size.*/
	gl.getIntegerv(GL_MAX_TEXTURE_SIZE, &m_max_texture_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv has failed");

	glw::GLenum not_matching_format					   = GL_RED;
	glw::GLenum not_matching_internalformat_compressed = GL_COMPRESSED_RED;

	/* 1D Compressed with a non matching format. We need to do all the allocation to get the correct image size */
	glw::GLuint to_1D_compressed_not_matching;

	gl.createTextures(GL_TEXTURE_1D, 1, &to_1D_compressed_not_matching);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	gl.bindTexture(GL_TEXTURE_1D, to_1D_compressed_not_matching);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

	gl.texImage1D(GL_TEXTURE_1D, 0, not_matching_internalformat_compressed, s_reference_width, 0, s_reference_format,
				  GL_UNSIGNED_BYTE, s_reference);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage1D has failed");

	is_compressed = 0;

	gl.getTexLevelParameteriv(GL_TEXTURE_1D, 0, GL_TEXTURE_COMPRESSED, &is_compressed);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTetTexLevelParameteriv has failed");

	if (is_compressed)
	{
		gl.getTexLevelParameteriv(GL_TEXTURE_1D, 0, GL_TEXTURE_INTERNAL_FORMAT, &m_not_matching_compressed_1D_format);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameteriv has failed");

		m_not_matching_compressed_1D_size = 0;

		gl.getTexLevelParameteriv(GL_TEXTURE_1D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE,
								  &m_not_matching_compressed_1D_size);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameteriv has failed");
	}

	gl.deleteTextures(1, &to_1D_compressed_not_matching);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	/* 2D Compressed with a non matching format. We need to do all the allocation to get the correct image size */
	glw::GLuint to_2D_compressed_not_matching;

	gl.createTextures(GL_TEXTURE_2D, 1, &to_2D_compressed_not_matching);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	gl.bindTexture(GL_TEXTURE_2D, to_2D_compressed_not_matching);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

	gl.texImage2D(GL_TEXTURE_2D, 0, not_matching_internalformat_compressed, s_reference_width, s_reference_height, 0,
				  not_matching_format, GL_UNSIGNED_BYTE, s_reference);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage2D has failed");

	is_compressed = 0;

	gl.getTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED, &is_compressed);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTetTexLevelParameteriv has failed");

	if (is_compressed)
	{
		gl.getTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &m_not_matching_compressed_2D_format);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameteriv has failed");

		m_not_matching_compressed_2D_size = 0;

		gl.getTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE,
								  &m_not_matching_compressed_2D_size);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameteriv has failed");
	}

	gl.deleteTextures(1, &to_2D_compressed_not_matching);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	/* 3D Compressed with a non matching format. We need to do all the allocation to get the correct image size */
	glw::GLuint to_3D_compressed_not_matching;

	gl.createTextures(GL_TEXTURE_3D, 1, &to_3D_compressed_not_matching);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	gl.bindTexture(GL_TEXTURE_3D, to_3D_compressed_not_matching);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

	gl.texImage3D(GL_TEXTURE_3D, 0, not_matching_internalformat_compressed, s_reference_width, s_reference_height,
				  s_reference_depth, 0, not_matching_format, GL_UNSIGNED_BYTE, s_reference);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage3D has failed");

	is_compressed = 0;

	gl.getTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_COMPRESSED, &is_compressed);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTetTexLevelParameteriv has failed");

	if (is_compressed)
	{
		gl.getTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_INTERNAL_FORMAT, &m_not_matching_compressed_3D_format);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameteriv has failed");

		m_not_matching_compressed_3D_size = 0;

		gl.getTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE,
								  &m_not_matching_compressed_3D_size);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexLevelParameteriv has failed");
	}

	gl.deleteTextures(1, &to_3D_compressed_not_matching);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");
}

/** @brief Test (negative) of TextureSubImage1D
 *
 *  @return Test result.
 */
bool SubImageErrorsTest::Test1D()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Result. */
	bool is_ok = true;

	/* Check that INVALID_OPERATION is generated by TextureSubImage1D if
	 texture is not the name of an existing texture object. */
	{
		gl.textureSubImage1D(m_to_invalid, 0, 0, s_reference_width, s_reference_format, s_reference_type, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage1D",
								  "texture is not the name of an existing texture object.");
	}

	/* Check that INVALID_ENUM is generated by TextureSubImage1D if format is
	 not an accepted format constant. */
	{
		gl.textureSubImage1D(m_to_1D, 0, 0, s_reference_width, m_format_invalid, s_reference_type, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureSubImage1D",
								  "format is not an accepted format constant.");
	}

	/* Check that INVALID_ENUM is generated by TextureSubImage1D if type is not
	 an accepted type constant. */
	{
		gl.textureSubImage1D(m_to_1D, 0, 0, s_reference_width, s_reference_format, m_type_invalid, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureSubImage1D",
								  "type is not an accepted type constant.");
	}

	/* Check that INVALID_VALUE is generated by TextureSubImage1D if level is
	 less than 0. */
	{
		gl.textureSubImage1D(m_to_1D, -1, 0, s_reference_width, s_reference_format, s_reference_type, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureSubImage1D", "level is less than 0.");
	}

	/* Check that INVALID_VALUE may be generated by TextureSubImage1D if level
	 is greater than log2 max, where max is the returned value of
	 MAX_TEXTURE_SIZE. */
	{
		gl.textureSubImage1D(m_to_1D, m_max_texture_size, 0, s_reference_width, s_reference_format, s_reference_type,
							 s_reference);
		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureSubImage1D",
							 "level is greater than log2 max, where max is the returned value of MAX_TEXTURE_SIZE.");
	}

	/* Check that INVALID_VALUE is generated by TextureSubImage1D if
	 xoffset<-b, or if (xoffset+width)>(w-b), where w is the TEXTURE_WIDTH,
	 and b is the width of the TEXTURE_BORDER of the texture image being
	 modified. Note that w includes twice the border width. */
	{
		gl.textureSubImage1D(m_to_1D, 0, -1, s_reference_width, s_reference_format, s_reference_type, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureSubImage1D",
								  "xoffset<-b, where b is the width of the TEXTURE_BORDER.");

		gl.textureSubImage1D(m_to_1D, 0, 1, s_reference_width + 1, s_reference_format, s_reference_type, s_reference);
		is_ok &= CheckErrorAndLog(
			m_context, GL_INVALID_VALUE, "glTextureSubImage1D",
			"(xoffset+width)>(w-b), where w is the TEXTURE_WIDTH, b is the width of the TEXTURE_BORDER.");
	}

	/*Check that INVALID_VALUE is generated by TextureSubImage1D if width is less than 0. */
	{
#ifndef TURN_OFF_SUB_IMAGE_ERRORS_TEST_OF_NEGATIVE_WIDTH_HEIGHT_OR_DEPTH
		gl.textureSubImage1D(m_to_1D, 0, 0, -1, s_reference_format, s_reference_type, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureSubImage1D", "width is less than 0.");
#endif
	}

	/* Check that INVALID_OPERATION is generated by TextureSubImage1D if type
	 is one of UNSIGNED_BYTE_3_3_2, UNSIGNED_BYTE_2_3_3_REV,
	 UNSIGNED_SHORT_5_6_5, or UNSIGNED_SHORT_5_6_5_REV and format is not RGB. */
	{
		gl.textureSubImage1D(m_to_1D, 0, 0, s_reference_width, s_reference_format, GL_UNSIGNED_BYTE_3_3_2, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage1D",
								  "type is UNSIGNED_BYTE_3_3_2 and format is not RGB.");

		gl.textureSubImage1D(m_to_1D, 0, 0, s_reference_width, s_reference_format, GL_UNSIGNED_BYTE_2_3_3_REV,
							 s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage1D",
								  "type is UNSIGNED_BYTE_2_3_3_REV and format is not RGB.");

		gl.textureSubImage1D(m_to_1D, 0, 0, s_reference_width, s_reference_format, GL_UNSIGNED_SHORT_5_6_5,
							 s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage1D",
								  "type is UNSIGNED_SHORT_5_6_5 and format is not RGB.");

		gl.textureSubImage1D(m_to_1D, 0, 0, s_reference_width, s_reference_format, GL_UNSIGNED_SHORT_5_6_5_REV,
							 s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage1D",
								  "type is UNSIGNED_SHORT_5_6_5_REV and format is not RGB.");
	}

	/* Check that INVALID_OPERATION is generated by TextureSubImage1D if type
	 is one of UNSIGNED_SHORT_4_4_4_4, UNSIGNED_SHORT_4_4_4_4_REV,
	 UNSIGNED_SHORT_5_5_5_1, UNSIGNED_SHORT_1_5_5_5_REV,
	 UNSIGNED_INT_8_8_8_8, UNSIGNED_INT_8_8_8_8_REV, UNSIGNED_INT_10_10_10_2,
	 or UNSIGNED_INT_2_10_10_10_REV and format is neither RGBA nor BGRA. */
	{
		gl.textureSubImage1D(m_to_1D, 0, 0, s_reference_width, s_reference_format, GL_UNSIGNED_SHORT_4_4_4_4,
							 s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage1D",
								  "type is UNSIGNED_SHORT_4_4_4_4 and format is neither RGBA nor BGRA.");

		gl.textureSubImage1D(m_to_1D, 0, 0, s_reference_width, s_reference_format, GL_UNSIGNED_SHORT_4_4_4_4_REV,
							 s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage1D",
								  "type is UNSIGNED_SHORT_4_4_4_4_REV and format is neither RGBA nor BGRA.");

		gl.textureSubImage1D(m_to_1D, 0, 0, s_reference_width, s_reference_format, GL_UNSIGNED_SHORT_5_5_5_1,
							 s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage1D",
								  "type is UNSIGNED_SHORT_5_5_5_1 and format is neither RGBA nor BGRA.");

		gl.textureSubImage1D(m_to_1D, 0, 0, s_reference_width, s_reference_format, GL_UNSIGNED_SHORT_1_5_5_5_REV,
							 s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage1D",
								  "type is UNSIGNED_SHORT_1_5_5_5_REV and format is neither RGBA nor BGRA.");

		gl.textureSubImage1D(m_to_1D, 0, 0, s_reference_width, s_reference_format, GL_UNSIGNED_INT_8_8_8_8,
							 s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage1D",
								  "type is UNSIGNED_INT_8_8_8_8 and format is neither RGBA nor BGRA.");

		gl.textureSubImage1D(m_to_1D, 0, 0, s_reference_width, s_reference_format, GL_UNSIGNED_INT_8_8_8_8_REV,
							 s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage1D",
								  "type is UNSIGNED_INT_8_8_8_8_REV and format is neither RGBA nor BGRA.");

		gl.textureSubImage1D(m_to_1D, 0, 0, s_reference_width, s_reference_format, GL_UNSIGNED_INT_10_10_10_2,
							 s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage1D",
								  "type is UNSIGNED_INT_10_10_10_2 and format is neither RGBA nor BGRA.");

		gl.textureSubImage1D(m_to_1D, 0, 0, s_reference_width, s_reference_format, GL_UNSIGNED_INT_2_10_10_10_REV,
							 s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage1D",
								  "type is UNSIGNED_INT_2_10_10_10_REV and format is neither RGBA nor BGRA.");
	}

	/* Check that INVALID_OPERATION is generated by TextureSubImage1D if a
	 non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target
	 and the buffer object's data store is currently mapped. */
	{
		gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, m_bo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");

		gl.mapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_READ_WRITE);

		if (GL_NO_ERROR == gl.getError())
		{
			gl.textureSubImage1D(m_to_1D, 0, 0, s_reference_width, s_reference_format, s_reference_type, NULL);
			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage1D",
									  "a non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target and "
									  "the buffer object's data store is currently mapped.");

			gl.unmapBuffer(GL_PIXEL_UNPACK_BUFFER);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");

			gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");
		}
	}

	/* Check that INVALID_OPERATION is generated by TextureSubImage1D if a
	 non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target
	 and the data would be unpacked from the buffer object such that the
	 memory reads required would exceed the data store size. */
	{
		gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, m_bo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");

		gl.textureSubImage1D(m_to_1D, 0, 0, s_reference_width, s_reference_format, s_reference_type,
							 (glw::GLubyte*)NULL + s_reference_size * 2);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage1D",
								  "a non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target and the "
								  "data would be unpacked from the buffer object such that the memory reads required "
								  "would exceed the data store size.");

		gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");
	}

	/* Check that INVALID_OPERATION is generated by TextureSubImage1D if a
	 non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target
	 and pixels is not evenly divisible into the number of bytes needed to
	 store in memory a datum indicated by type. */
	{
		gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, m_bo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");

		gl.textureSubImage1D(m_to_1D, 0, 0, s_reference_width, s_reference_format, s_reference_type,
							 (glw::GLubyte*)NULL + 1);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage1D",
								  "a non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target and pixels "
								  "is not evenly divisible into the number of bytes needed to store in memory a datum "
								  "indicated by type.");

		gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");
	}

	return is_ok;
}

/** @brief Test (negative) of TextureSubImage2D
 *
 *  @return Test result.
 */
bool SubImageErrorsTest::Test2D()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Result. */
	bool is_ok = true;

	/* Check that INVALID_OPERATION is generated by TextureSubImage2D if
	 texture is not the name of an existing texture object. */
	{
		gl.textureSubImage2D(m_to_invalid, 0, 0, 0, s_reference_width, s_reference_height, s_reference_format,
							 s_reference_type, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage2D",
								  "texture is not the name of an existing texture object.");
	}

	/* Check that INVALID_ENUM is generated by TextureSubImage2D if format is
	 not an accepted format constant. */
	{
		gl.textureSubImage2D(m_to_2D, 0, 0, 0, s_reference_width, s_reference_height, m_format_invalid,
							 s_reference_type, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureSubImage2D",
								  "format is not an accepted format constant.");
	}

	/* Check that INVALID_ENUM is generated by TextureSubImage2D if type is not
	 an accepted type constant. */
	{
		gl.textureSubImage2D(m_to_2D, 0, 0, 0, s_reference_width, s_reference_height, s_reference_format,
							 m_type_invalid, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureSubImage2D",
								  "type is not an accepted type constant.");
	}

	/* Check that INVALID_VALUE is generated by TextureSubImage2D if level is
	 less than 0. */
	{
		gl.textureSubImage2D(m_to_2D, -1, 0, 0, s_reference_width, s_reference_height, s_reference_format,
							 s_reference_type, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureSubImage2D", "level is less than 0.");
	}

	/* Check that INVALID_VALUE may be generated by TextureSubImage2D if level
	 is greater than log2 max, where max is the returned value of
	 MAX_TEXTURE_SIZE. */
	{
		gl.textureSubImage2D(m_to_2D, m_max_texture_size, 0, 0, s_reference_width, s_reference_height,
							 s_reference_format, s_reference_type, s_reference);
		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureSubImage2D",
							 "level is greater than log2 max, where max is the returned value of MAX_TEXTURE_SIZE.");
	}

	/* Check that INVALID_VALUE may be generated by TextureSubImage2D if level
	 is greater than log2 max, where max is the returned value of
	 MAX_TEXTURE_SIZE.
	 Check that INVALID_VALUE is generated by TextureSubImage2D if
	 xoffset<-b, (xoffset+width)>(w-b), yoffset<-b, or
	 (yoffset+height)>(h-b), where w is the TEXTURE_WIDTH, h is the
	 TEXTURE_HEIGHT, and b is the border width of the texture image being
	 modified. Note that w and h include twice the border width. */
	{
		gl.textureSubImage2D(m_to_2D, 0, -1, 0, s_reference_width, s_reference_height, s_reference_format,
							 s_reference_type, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureSubImage2D",
								  "xoffset<-b, where b is the width of the TEXTURE_BORDER.");

		gl.textureSubImage2D(m_to_2D, 0, 1, 0, s_reference_width + 1, s_reference_height, s_reference_format,
							 s_reference_type, s_reference);
		is_ok &= CheckErrorAndLog(
			m_context, GL_INVALID_VALUE, "glTextureSubImage2D",
			"(xoffset+width)>(w-b), where w is the TEXTURE_WIDTH, b is the width of the TEXTURE_BORDER.");

		gl.textureSubImage2D(m_to_2D, 0, 0, -1, s_reference_width, s_reference_height, s_reference_format,
							 s_reference_type, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureSubImage2D",
								  "yoffset<-b, where b is the height of the TEXTURE_BORDER.");

		gl.textureSubImage2D(m_to_2D, 0, 0, 1, s_reference_width + 1, s_reference_height, s_reference_format,
							 s_reference_type, s_reference);
		is_ok &= CheckErrorAndLog(
			m_context, GL_INVALID_VALUE, "glTextureSubImage2D",
			"(yoffset+height)>(h-b), where h is the TEXTURE_HEIGHT, b is the width of the TEXTURE_BORDER.");
	}

	/*Check that INVALID_VALUE is generated by TextureSubImage2D if width or height is less than 0. */
	{
#ifndef TURN_OFF_SUB_IMAGE_ERRORS_TEST_OF_NEGATIVE_WIDTH_HEIGHT_OR_DEPTH
		gl.textureSubImage2D(m_to_2D, 0, 0, 0, -1, s_reference_height, s_reference_format, s_reference_type,
							 s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureSubImage2D", "width is less than 0.");

		gl.textureSubImage2D(m_to_2D, 0, 0, 0, s_reference_width, -1, s_reference_format, s_reference_type,
							 s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureSubImage2D", "height is less than 0.");
#endif
	}

	/* Check that INVALID_OPERATION is generated by TextureSubImage2D if type
	 is one of UNSIGNED_BYTE_3_3_2, UNSIGNED_BYTE_2_3_3_REV,
	 UNSIGNED_SHORT_5_6_5, or UNSIGNED_SHORT_5_6_5_REV and format is not RGB. */
	{
		gl.textureSubImage2D(m_to_2D, 0, 0, 0, s_reference_width, s_reference_height, s_reference_format,
							 GL_UNSIGNED_BYTE_3_3_2, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage2D",
								  "type is UNSIGNED_BYTE_3_3_2 and format is not RGB.");

		gl.textureSubImage2D(m_to_2D, 0, 0, 0, s_reference_width, s_reference_height, s_reference_format,
							 GL_UNSIGNED_BYTE_2_3_3_REV, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage2D",
								  "type is UNSIGNED_BYTE_2_3_3_REV and format is not RGB.");

		gl.textureSubImage2D(m_to_2D, 0, 0, 0, s_reference_width, s_reference_height, s_reference_format,
							 GL_UNSIGNED_SHORT_5_6_5, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage2D",
								  "type is UNSIGNED_SHORT_5_6_5 and format is not RGB.");

		gl.textureSubImage2D(m_to_2D, 0, 0, 0, s_reference_width, s_reference_height, s_reference_format,
							 GL_UNSIGNED_SHORT_5_6_5_REV, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage2D",
								  "type is UNSIGNED_SHORT_5_6_5_REV and format is not RGB.");
	}

	/* Check that INVALID_OPERATION is generated by TextureSubImage2D if type
	 is one of UNSIGNED_SHORT_4_4_4_4, UNSIGNED_SHORT_4_4_4_4_REV,
	 UNSIGNED_SHORT_5_5_5_1, UNSIGNED_SHORT_1_5_5_5_REV,
	 UNSIGNED_INT_8_8_8_8, UNSIGNED_INT_8_8_8_8_REV, UNSIGNED_INT_10_10_10_2,
	 or UNSIGNED_INT_2_10_10_10_REV and format is neither RGBA nor BGRA. */
	{
		gl.textureSubImage2D(m_to_2D, 0, 0, 0, s_reference_width, s_reference_height, s_reference_format,
							 GL_UNSIGNED_SHORT_4_4_4_4, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage2D",
								  "type is UNSIGNED_SHORT_4_4_4_4 and format is neither RGBA nor BGRA.");

		gl.textureSubImage2D(m_to_2D, 0, 0, 0, s_reference_width, s_reference_height, s_reference_format,
							 GL_UNSIGNED_SHORT_4_4_4_4_REV, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage2D",
								  "type is UNSIGNED_SHORT_4_4_4_4_REV and format is neither RGBA nor BGRA.");

		gl.textureSubImage2D(m_to_2D, 0, 0, 0, s_reference_width, s_reference_height, s_reference_format,
							 GL_UNSIGNED_SHORT_5_5_5_1, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage2D",
								  "type is UNSIGNED_SHORT_5_5_5_1 and format is neither RGBA nor BGRA.");

		gl.textureSubImage2D(m_to_2D, 0, 0, 0, s_reference_width, s_reference_height, s_reference_format,
							 GL_UNSIGNED_SHORT_1_5_5_5_REV, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage2D",
								  "type is UNSIGNED_SHORT_1_5_5_5_REV and format is neither RGBA nor BGRA.");

		gl.textureSubImage2D(m_to_2D, 0, 0, 0, s_reference_width, s_reference_height, s_reference_format,
							 GL_UNSIGNED_INT_8_8_8_8, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage2D",
								  "type is UNSIGNED_INT_8_8_8_8 and format is neither RGBA nor BGRA.");

		gl.textureSubImage2D(m_to_2D, 0, 0, 0, s_reference_width, s_reference_height, s_reference_format,
							 GL_UNSIGNED_INT_8_8_8_8_REV, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage2D",
								  "type is UNSIGNED_INT_8_8_8_8_REV and format is neither RGBA nor BGRA.");

		gl.textureSubImage2D(m_to_2D, 0, 0, 0, s_reference_width, s_reference_height, s_reference_format,
							 GL_UNSIGNED_INT_10_10_10_2, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage2D",
								  "type is UNSIGNED_INT_10_10_10_2 and format is neither RGBA nor BGRA.");

		gl.textureSubImage2D(m_to_2D, 0, 0, 0, s_reference_width, s_reference_height, s_reference_format,
							 GL_UNSIGNED_INT_2_10_10_10_REV, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage2D",
								  "type is UNSIGNED_INT_2_10_10_10_REV and format is neither RGBA nor BGRA.");
	}

	/* Check that INVALID_OPERATION is generated by TextureSubImage2D if a
	 non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target
	 and the buffer object's data store is currently mapped. */
	{
		gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, m_bo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");

		gl.mapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_READ_WRITE);

		if (GL_NO_ERROR == gl.getError())
		{
			gl.textureSubImage2D(m_to_2D, 0, 0, 0, s_reference_width, s_reference_height, s_reference_format,
								 s_reference_type, NULL);
			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage2D",
									  "a non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target and "
									  "the buffer object's data store is currently mapped.");

			gl.unmapBuffer(GL_PIXEL_UNPACK_BUFFER);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");

			gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");
		}
	}

	/* Check that INVALID_OPERATION is generated by TextureSubImage2D if a
	 non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target
	 and the data would be unpacked from the buffer object such that the
	 memory reads required would exceed the data store size. */
	{
		gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, m_bo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");

		gl.textureSubImage2D(m_to_2D, 0, 0, 0, s_reference_width, s_reference_height, s_reference_format,
							 s_reference_type, (glw::GLubyte*)NULL + s_reference_size * 2);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage2D",
								  "a non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target and the "
								  "data would be unpacked from the buffer object such that the memory reads required "
								  "would exceed the data store size.");

		gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");
	}

	/* Check that INVALID_OPERATION is generated by TextureSubImage2D if a
	 non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target
	 and pixels is not evenly divisible into the number of bytes needed to
	 store in memory a datum indicated by type. */
	{
		gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, m_bo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");

		gl.textureSubImage2D(m_to_2D, 0, 0, 0, s_reference_width, s_reference_height, s_reference_format,
							 s_reference_type, (glw::GLubyte*)NULL + 1);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage2D",
								  "a non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target and pixels "
								  "is not evenly divisible into the number of bytes needed to store in memory a datum "
								  "indicated by type.");

		gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");
	}

	return is_ok;
}

/** @brief Test (negative) of TextureSubImage3D
 *
 *  @return Test result.
 */
bool SubImageErrorsTest::Test3D()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Result. */
	bool is_ok = true;

	/* Check that INVALID_OPERATION is generated by TextureSubImage3D if
	 texture is not the name of an existing texture object. */
	{
		gl.textureSubImage3D(m_to_invalid, 0, 0, 0, 0, s_reference_width, s_reference_height, s_reference_depth,
							 s_reference_format, s_reference_type, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage3D",
								  "texture is not the name of an existing texture object.");
	}

	/* Check that INVALID_ENUM is generated by TextureSubImage3D if format is
	 not an accepted format constant. */
	{
		gl.textureSubImage3D(m_to_3D, 0, 0, 0, 0, s_reference_width, s_reference_height, s_reference_depth,
							 m_format_invalid, s_reference_type, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureSubImage3D",
								  "format is not an accepted format constant.");
	}

	/* Check that INVALID_ENUM is generated by TextureSubImage3D if type is not
	 an accepted type constant. */
	{
		gl.textureSubImage3D(m_to_3D, 0, 0, 0, 0, s_reference_width, s_reference_height, s_reference_depth,
							 s_reference_format, m_type_invalid, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureSubImage3D",
								  "type is not an accepted type constant.");
	}

	/* Check that INVALID_VALUE is generated by TextureSubImage3D if level is
	 less than 0. */
	{
		gl.textureSubImage3D(m_to_3D, -1, 0, 0, 0, s_reference_width, s_reference_height, s_reference_depth,
							 s_reference_format, s_reference_type, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureSubImage3D", "level is less than 0.");
	}

	/* Check that INVALID_VALUE may be generated by TextureSubImage1D if level
	 is greater than log2 max, where max is the returned value of
	 MAX_TEXTURE_SIZE. */
	{
		gl.textureSubImage3D(m_to_3D, m_max_texture_size, 0, 0, 0, s_reference_width, s_reference_height,
							 s_reference_depth, s_reference_format, s_reference_type, s_reference);
		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureSubImage3D",
							 "level is greater than log2 max, where max is the returned value of MAX_TEXTURE_SIZE.");
	}

	/* Check that INVALID_VALUE is generated by TextureSubImage3D if
	 xoffset<-b, (xoffset+width)>(w-b), yoffset<-b, or
	 (yoffset+height)>(h-b), or zoffset<-b, or (zoffset+depth)>(d-b), where w
	 is the TEXTURE_WIDTH, h is the TEXTURE_HEIGHT, d is the TEXTURE_DEPTH
	 and b is the border width of the texture image being modified. Note
	 that w, h, and d include twice the border width. */
	{
		gl.textureSubImage3D(m_to_3D, 0, -1, 0, 0, s_reference_width, s_reference_height, s_reference_depth,
							 s_reference_format, s_reference_type, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureSubImage3D",
								  "xoffset<-b, where b is the width of the TEXTURE_BORDER.");

		gl.textureSubImage3D(m_to_3D, 0, 1, 0, 0, s_reference_width + 1, s_reference_height, s_reference_depth,
							 s_reference_format, s_reference_type, s_reference);
		is_ok &= CheckErrorAndLog(
			m_context, GL_INVALID_VALUE, "glTextureSubImage3D",
			"(xoffset+width)>(w-b), where w is the TEXTURE_WIDTH, b is the width of the TEXTURE_BORDER.");

		gl.textureSubImage3D(m_to_3D, 0, 0, -1, 0, s_reference_width, s_reference_height, s_reference_depth,
							 s_reference_format, s_reference_type, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureSubImage3D",
								  "yoffset<-b, where b is the width of the TEXTURE_BORDER.");

		gl.textureSubImage3D(m_to_3D, 0, 0, 1, 0, s_reference_width + 1, s_reference_height, s_reference_depth,
							 s_reference_format, s_reference_type, s_reference);
		is_ok &= CheckErrorAndLog(
			m_context, GL_INVALID_VALUE, "glTextureSubImage3D",
			"(yoffset+height)>(h-b), where h is the TEXTURE_HEIGHT, b is the width of the TEXTURE_BORDER.");

		gl.textureSubImage3D(m_to_3D, 0, 0, 0, -1, s_reference_width, s_reference_height, s_reference_depth,
							 s_reference_format, s_reference_type, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureSubImage3D",
								  "zoffset<-b, where b is the depth of the TEXTURE_BORDER.");

		gl.textureSubImage3D(m_to_3D, 0, 0, 0, 1, s_reference_width + 1, s_reference_height, s_reference_depth,
							 s_reference_format, s_reference_type, s_reference);
		is_ok &= CheckErrorAndLog(
			m_context, GL_INVALID_VALUE, "glTextureSubImage3D",
			"(zoffset+width)>(d-b), where d is the TEXTURE_DEPTH, b is the width of the TEXTURE_BORDER.");
	}

	/*Check that INVALID_VALUE is generated by TextureSubImage3D if width or height or depth is less than 0. */
	{
#ifndef TURN_OFF_SUB_IMAGE_ERRORS_TEST_OF_NEGATIVE_WIDTH_HEIGHT_OR_DEPTH
		gl.textureSubImage3D(m_to_3D, 0, 0, 0, 0, -1, s_reference_height, s_reference_depth, s_reference_format,
							 s_reference_type, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureSubImage1D", "width is less than 0.");

		gl.textureSubImage3D(m_to_3D, 0, 0, 0, 0, s_reference_width, -1, s_reference_depth, s_reference_format,
							 s_reference_type, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureSubImage1D", "height is less than 0.");

		gl.textureSubImage3D(m_to_3D, 0, 0, 0, 0, s_reference_width, s_reference_height, -1, s_reference_format,
							 s_reference_type, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureSubImage1D", "depth is less than 0.");
#endif
	}

	/* Check that INVALID_OPERATION is generated by TextureSubImage3D if type
	 is one of UNSIGNED_BYTE_3_3_2, UNSIGNED_BYTE_2_3_3_REV,
	 UNSIGNED_SHORT_5_6_5, or UNSIGNED_SHORT_5_6_5_REV and format is not RGB. */
	{
		gl.textureSubImage3D(m_to_3D, 0, 0, 0, 0, s_reference_width, s_reference_height, s_reference_depth,
							 s_reference_format, GL_UNSIGNED_BYTE_3_3_2, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage3D",
								  "type is UNSIGNED_BYTE_3_3_2 and format is not RGB.");

		gl.textureSubImage3D(m_to_3D, 0, 0, 0, 0, s_reference_width, s_reference_height, s_reference_depth,
							 s_reference_format, GL_UNSIGNED_BYTE_2_3_3_REV, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage3D",
								  "type is UNSIGNED_BYTE_2_3_3_REV and format is not RGB.");

		gl.textureSubImage3D(m_to_3D, 0, 0, 0, 0, s_reference_width, s_reference_height, s_reference_depth,
							 s_reference_format, GL_UNSIGNED_SHORT_5_6_5, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage3D",
								  "type is UNSIGNED_SHORT_5_6_5 and format is not RGB.");

		gl.textureSubImage3D(m_to_3D, 0, 0, 0, 0, s_reference_width, s_reference_height, s_reference_depth,
							 s_reference_format, GL_UNSIGNED_SHORT_5_6_5_REV, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage3D",
								  "type is UNSIGNED_SHORT_5_6_5_REV and format is not RGB.");
	}

	/* Check that INVALID_OPERATION is generated by TextureSubImage3D if type
	 is one of UNSIGNED_SHORT_4_4_4_4, UNSIGNED_SHORT_4_4_4_4_REV,
	 UNSIGNED_SHORT_5_5_5_1, UNSIGNED_SHORT_1_5_5_5_REV,
	 UNSIGNED_INT_8_8_8_8, UNSIGNED_INT_8_8_8_8_REV, UNSIGNED_INT_10_10_10_2,
	 or UNSIGNED_INT_2_10_10_10_REV and format is neither RGBA nor BGRA. */
	{
		gl.textureSubImage3D(m_to_3D, 0, 0, 0, 0, s_reference_width, s_reference_height, s_reference_depth,
							 s_reference_format, GL_UNSIGNED_SHORT_4_4_4_4, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage3D",
								  "type is UNSIGNED_SHORT_4_4_4_4 and format is neither RGBA nor BGRA.");

		gl.textureSubImage3D(m_to_3D, 0, 0, 0, 0, s_reference_width, s_reference_height, s_reference_depth,
							 s_reference_format, GL_UNSIGNED_SHORT_4_4_4_4_REV, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage3D",
								  "type is UNSIGNED_SHORT_4_4_4_4_REV and format is neither RGBA nor BGRA.");

		gl.textureSubImage3D(m_to_3D, 0, 0, 0, 0, s_reference_width, s_reference_height, s_reference_depth,
							 s_reference_format, GL_UNSIGNED_SHORT_5_5_5_1, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage3D",
								  "type is UNSIGNED_SHORT_5_5_5_1 and format is neither RGBA nor BGRA.");

		gl.textureSubImage3D(m_to_3D, 0, 0, 0, 0, s_reference_width, s_reference_height, s_reference_depth,
							 s_reference_format, GL_UNSIGNED_SHORT_1_5_5_5_REV, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage3D",
								  "type is UNSIGNED_SHORT_1_5_5_5_REV and format is neither RGBA nor BGRA.");

		gl.textureSubImage3D(m_to_3D, 0, 0, 0, 0, s_reference_width, s_reference_height, s_reference_depth,
							 s_reference_format, GL_UNSIGNED_INT_8_8_8_8, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage3D",
								  "type is UNSIGNED_INT_8_8_8_8 and format is neither RGBA nor BGRA.");

		gl.textureSubImage3D(m_to_3D, 0, 0, 0, 0, s_reference_width, s_reference_height, s_reference_depth,
							 s_reference_format, GL_UNSIGNED_INT_8_8_8_8_REV, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage3D",
								  "type is UNSIGNED_INT_8_8_8_8_REV and format is neither RGBA nor BGRA.");

		gl.textureSubImage3D(m_to_3D, 0, 0, 0, 0, s_reference_width, s_reference_height, s_reference_depth,
							 s_reference_format, GL_UNSIGNED_INT_10_10_10_2, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage3D",
								  "type is UNSIGNED_INT_10_10_10_2 and format is neither RGBA nor BGRA.");

		gl.textureSubImage3D(m_to_3D, 0, 0, 0, 0, s_reference_width, s_reference_height, s_reference_depth,
							 s_reference_format, GL_UNSIGNED_INT_2_10_10_10_REV, s_reference);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage3D",
								  "type is UNSIGNED_INT_2_10_10_10_REV and format is neither RGBA nor BGRA.");
	}

	/* Check that INVALID_OPERATION is generated by TextureSubImage3D if a
	 non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target
	 and the buffer object's data store is currently mapped. */
	{
		gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, m_bo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");

		gl.mapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_READ_WRITE);

		if (GL_NO_ERROR == gl.getError())
		{
			gl.textureSubImage3D(m_to_3D, 0, 0, 0, 0, s_reference_width, s_reference_height, s_reference_depth,
								 s_reference_format, s_reference_type, NULL);
			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage3D",
									  "a non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target and "
									  "the buffer object's data store is currently mapped.");

			gl.unmapBuffer(GL_PIXEL_UNPACK_BUFFER);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");

			gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");
		}
	}

	/* Check that INVALID_OPERATION is generated by TextureSubImage3D if a
	 non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target
	 and the data would be unpacked from the buffer object such that the
	 memory reads required would exceed the data store size. */
	{
		gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, m_bo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");

		gl.textureSubImage3D(m_to_3D, 0, 0, 0, 0, s_reference_width, s_reference_height, s_reference_depth,
							 s_reference_format, s_reference_type, (glw::GLubyte*)NULL + s_reference_size * 2);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage3D",
								  "a non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target and the "
								  "data would be unpacked from the buffer object such that the memory reads required "
								  "would exceed the data store size.");

		gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");
	}

	/* Check that INVALID_OPERATION is generated by TextureSubImage3D if a
	 non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target
	 and pixels is not evenly divisible into the number of bytes needed to
	 store in memory a datum indicated by type. */
	{
		gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, m_bo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");

		gl.textureSubImage3D(m_to_3D, 0, 0, 0, 0, s_reference_width, s_reference_height, s_reference_depth,
							 s_reference_format, s_reference_type, (glw::GLubyte*)NULL + 1);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureSubImage3D",
								  "a non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target and pixels "
								  "is not evenly divisible into the number of bytes needed to store in memory a datum "
								  "indicated by type.");

		gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");
	}

	return is_ok;
}

/** @brief Test (negative) of TextureSubImage1DCompressed
 *
 *  @return Test result.
 */
bool SubImageErrorsTest::Test1DCompressed()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Result. */
	bool is_ok = true;

	/* Do tests only if compressed 1D textures are supported. */
	if (DE_NULL != m_reference_compressed_1D)
	{
		/* Check that INVALID_OPERATION is generated by CompressedTextureSubImage1D
		 if texture is not the name of an existing texture object. */
		{
			gl.compressedTextureSubImage1D(m_to_invalid, 0, 0, s_reference_width, m_reference_compressed_1D_format,
										   m_reference_compressed_1D_size, m_reference_compressed_1D);
			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glCompressedTextureSubImage1D",
									  "texture is not the name of an existing texture object.");
		}

		/* Check that INVALID_ENUM is generated by CompressedTextureSubImage1D if
		 internalformat is not one of the generic compressed internal formats:
		 COMPRESSED_RED, COMPRESSED_RG, COMPRESSED_RGB, COMPRESSED_RGBA.
		 COMPRESSED_SRGB, or COMPRESSED_SRGB_ALPHA. */
		{
			/* GL_COMPRESSED_RG_RGTC2 is not 1D as specification says. */
			gl.compressedTextureSubImage1D(m_to_1D_compressed, 0, 0, s_reference_width, GL_COMPRESSED_RG_RGTC2,
										   m_reference_compressed_1D_size, m_reference_compressed_1D);
			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glCompressedTextureSubImage1D",
									  "internalformat is of the generic compressed internal formats: COMPRESSED_RED, "
									  "COMPRESSED_RG, COMPRESSED_RGB, COMPRESSED_RGBA. COMPRESSED_SRGB, or "
									  "COMPRESSED_SRGB_ALPHA.");
		}

		/* Check that INVALID_OPERATION is generated if format does not match the
		 internal format of the texture image being modified, since these
		 commands do not provide for image format conversion. */
		{
			gl.compressedTextureSubImage1D(m_to_1D_compressed, 0, 0, s_reference_width,
										   m_not_matching_compressed_1D_format, m_not_matching_compressed_1D_size,
										   m_reference_compressed_1D);
			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glCompressedTextureSubImage1D",
									  "format does not match the internal format of the texture image being modified, "
									  "since these commands do not provide for image format conversion.");
		}

		/* Check that INVALID_VALUE is generated by CompressedTextureSubImage1D if
		 imageSize is not consistent with the format, dimensions, and contents of
		 the specified compressed image data. */
		{
			gl.compressedTextureSubImage1D(m_to_1D_compressed, 0, 0, s_reference_width,
										   m_reference_compressed_1D_format, m_reference_compressed_1D_size - 1,
										   m_reference_compressed_1D);
			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glCompressedTextureSubImage1D",
									  "imageSize is not consistent with the format, dimensions, and contents of the "
									  "specified compressed image data.");
		}

		/* Check that INVALID_OPERATION is generated by CompressedTextureSubImage1D
		 if a non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER
		 target and the buffer object's data store is currently mapped. */
		{
			gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, m_bo);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");

			gl.mapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_READ_WRITE);

			if (GL_NO_ERROR == gl.getError())
			{
				gl.compressedTextureSubImage1D(m_to_1D_compressed, 0, 0, s_reference_width,
											   m_reference_compressed_1D_format, m_reference_compressed_1D_size, NULL);
				is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glCompressedTextureSubImage1D",
										  "a non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target "
										  "and the buffer object's data store is currently mapped.");

				gl.unmapBuffer(GL_PIXEL_UNPACK_BUFFER);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");

				gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");
			}
		}

		/* Check that INVALID_OPERATION is generated by CompressedTextureSubImage1D
		 if a non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER
		 target and the data would be unpacked from the buffer object such that
		 the memory reads required would exceed the data store size. */
		{
			gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, m_bo);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");

			gl.compressedTextureSubImage1D(m_to_1D_compressed, 0, 0, s_reference_width,
										   m_reference_compressed_1D_format, m_reference_compressed_1D_size,
										   (glw::GLubyte*)NULL + s_reference_size * 2);
			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glCompressedTextureSubImage1D",
									  "a non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target and "
									  "the buffer object's data store is currently mapped.");

			gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");
		}
	}

	return is_ok;
}

/** @brief Test (negative) of TextureSubImage2DCompressed
 *
 *  @return Test result.
 */
bool SubImageErrorsTest::Test2DCompressed()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Result. */
	bool is_ok = true;

	/* Do tests only if compressed 2D textures are supported. */
	if (DE_NULL != m_reference_compressed_2D)
	{
		/* Check that INVALID_OPERATION is generated by CompressedTextureSubImage2D
		 if texture is not the name of an existing texture object. */
		{
			gl.compressedTextureSubImage2D(m_to_invalid, 0, 0, 0, s_reference_width, s_reference_height,
										   m_reference_compressed_2D_format, m_reference_compressed_2D_size,
										   m_reference_compressed_2D);
			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glCompressedTextureSubImage2D",
									  "texture is not the name of an existing texture object.");
		}

		/* Check that INVALID_ENUM is generated by CompressedTextureSubImage2D if
		 internalformat is of the generic compressed internal formats:
		 COMPRESSED_RED, COMPRESSED_RG, COMPRESSED_RGB, COMPRESSED_RGBA.
		 COMPRESSED_SRGB, or COMPRESSED_SRGB_ALPHA. */
		{
			gl.compressedTextureSubImage2D(m_to_2D_compressed, 0, 0, 0, s_reference_width, s_reference_height,
										   GL_COMPRESSED_RG, m_reference_compressed_2D_size, m_reference_compressed_2D);
			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glCompressedTextureSubImage2D",
									  "internalformat is of the generic compressed internal formats: COMPRESSED_RED, "
									  "COMPRESSED_RG, COMPRESSED_RGB, COMPRESSED_RGBA. COMPRESSED_SRGB, or "
									  "COMPRESSED_SRGB_ALPHA.");
		}

		/* Check that INVALID_OPERATION is generated if format does not match the
		 internal format of the texture image being modified, since these
		 commands do not provide for image format conversion. */
		{
			gl.compressedTextureSubImage2D(m_to_2D_compressed, 0, 0, 0, s_reference_width, s_reference_height,
										   m_not_matching_compressed_2D_format, m_not_matching_compressed_2D_size,
										   m_reference_compressed_2D);
			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glCompressedTextureSubImage2D",
									  "format does not match the internal format of the texture image being modified, "
									  "since these commands do not provide for image format conversion.");
		}

		/* Check that INVALID_VALUE is generated by CompressedTextureSubImage2D if
		 imageSize is not consistent with the format, dimensions, and contents of
		 the specified compressed image data. */
		{
			gl.compressedTextureSubImage2D(m_to_2D_compressed, 0, 0, 0, s_reference_width, s_reference_height,
										   m_reference_compressed_2D_format, m_reference_compressed_2D_size - 1,
										   m_reference_compressed_2D);
			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glCompressedTextureSubImage2D",
									  "imageSize is not consistent with the format, dimensions, and contents of the "
									  "specified compressed image data.");
		}

		/* Check that INVALID_OPERATION is generated by CompressedTextureSubImage2D
		 if a non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER
		 target and the buffer object's data store is currently mapped. */
		{
			gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, m_bo);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");

			gl.mapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_READ_WRITE);

			if (GL_NO_ERROR == gl.getError())
			{
				gl.compressedTextureSubImage2D(m_to_2D_compressed, 0, 0, 0, s_reference_width, s_reference_height,
											   m_reference_compressed_2D_format, m_reference_compressed_2D_size, NULL);
				is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glCompressedTextureSubImage2D",
										  "a non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target "
										  "and the buffer object's data store is currently mapped.");

				gl.unmapBuffer(GL_PIXEL_UNPACK_BUFFER);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");

				gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");
			}
		}

		/* Check that INVALID_OPERATION is generated by CompressedTextureSubImage2D
		 if a non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER
		 target and the data would be unpacked from the buffer object such that
		 the memory reads required would exceed the data store size. */
		{
			gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, m_bo);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");

			gl.compressedTextureSubImage2D(m_to_2D_compressed, 0, 0, 0, s_reference_width, s_reference_height,
										   m_reference_compressed_2D_format, m_reference_compressed_2D_size,
										   (glw::GLubyte*)NULL + s_reference_size * 2);
			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glCompressedTextureSubImage2D",
									  "a non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target and "
									  "the buffer object's data store is currently mapped.");

			gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");
		}

		/* Check that INVALID_OPERATION is generated by CompressedTextureSubImage2D
		 if the effective target is TEXTURE_RECTANGLE. */
		if (DE_NULL !=
			m_reference_compressed_rectangle) /* Do test only if rectangle compressed texture is supported by the implementation. */
		{
			gl.compressedTextureSubImage2D(m_to_rectangle_compressed, 0, 0, 0, s_reference_width, s_reference_height,
										   m_reference_compressed_rectangle_format,
										   m_reference_compressed_rectangle_size, m_reference_compressed_rectangle);
			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glCompressedTextureSubImage2D",
									  "texture is not the name of an existing texture object.");
		}
	}

	return is_ok;
}

/** @brief Test (negative) of TextureSubImage3DCompressed
 *
 *  @return Test result.
 */
bool SubImageErrorsTest::Test3DCompressed()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Result. */
	bool is_ok = true;

	/* Do tests only if compressed 3D textures are supported. */
	if (DE_NULL != m_reference_compressed_3D)
	{
		/* Check that INVALID_OPERATION is generated by CompressedTextureSubImage3D
		 if texture is not the name of an existing texture object. */
		{
			gl.compressedTextureSubImage3D(m_to_invalid, 0, 0, 0, 0, s_reference_width, s_reference_height,
										   s_reference_depth, m_reference_compressed_3D_format,
										   m_reference_compressed_3D_size, m_reference_compressed_3D);
			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glCompressedTextureSubImage3D",
									  "texture is not the name of an existing texture object.");
		}

		/* Check that INVALID_ENUM is generated by CompressedTextureSubImage3D if
		 internalformat is of the generic compressed internal formats:
		 COMPRESSED_RED, COMPRESSED_RG, COMPRESSED_RGB, COMPRESSED_RGBA.
		 COMPRESSED_SRGB, or COMPRESSED_SRGB_ALPHA. */
		{
			gl.compressedTextureSubImage3D(m_to_3D_compressed, 0, 0, 0, 0, s_reference_width, s_reference_height,
										   s_reference_depth, GL_COMPRESSED_RG, m_reference_compressed_3D_size,
										   m_reference_compressed_3D);
			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glCompressedTextureSubImage3D",
									  "internalformat is of the generic compressed internal formats: COMPRESSED_RED, "
									  "COMPRESSED_RG, COMPRESSED_RGB, COMPRESSED_RGBA. COMPRESSED_SRGB, or "
									  "COMPRESSED_SRGB_ALPHA.");
		}

		/* Check that INVALID_OPERATION is generated if format does not match the
		 internal format of the texture image being modified, since these
		 commands do not provide for image format conversion. */
		{
			gl.compressedTextureSubImage3D(m_to_3D_compressed, 0, 0, 0, 0, s_reference_width, s_reference_height,
										   s_reference_depth, m_not_matching_compressed_3D_format,
										   m_not_matching_compressed_3D_size, m_reference_compressed_3D);
			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glCompressedTextureSubImage3D",
									  "format does not match the internal format of the texture image being modified, "
									  "since these commands do not provide for image format conversion.");
		}

		/* Check that INVALID_VALUE is generated by CompressedTextureSubImage3D if
		 imageSize is not consistent with the format, dimensions, and contents of
		 the specified compressed image data. */
		{
			gl.compressedTextureSubImage3D(m_to_3D_compressed, 0, 0, 0, 0, s_reference_width, s_reference_height,
										   s_reference_depth, m_reference_compressed_3D_format,
										   m_reference_compressed_3D_size - 1, m_reference_compressed_3D);
			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glCompressedTextureSubImage3D",
									  "imageSize is not consistent with the format, dimensions, and contents of the "
									  "specified compressed image data.");
		}

		/* Check that INVALID_OPERATION is generated by CompressedTextureSubImage3D
		 if a non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER
		 target and the buffer object's data store is currently mapped. */
		{
			gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, m_bo);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");

			gl.mapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_READ_WRITE);

			if (GL_NO_ERROR == gl.getError())
			{
				gl.compressedTextureSubImage3D(m_to_3D_compressed, 0, 0, 0, 0, s_reference_width, s_reference_height,
											   s_reference_depth, m_reference_compressed_3D_format,
											   m_reference_compressed_3D_size, NULL);
				is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glCompressedTextureSubImage3D",
										  "a non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target "
										  "and the buffer object's data store is currently mapped.");

				gl.unmapBuffer(GL_PIXEL_UNPACK_BUFFER);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");

				gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");
			}
		}

		/* Check that INVALID_OPERATION is generated by CompressedTextureSubImage3D
		 if a non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER
		 target and the data would be unpacked from the buffer object such that
		 the memory reads required would exceed the data store size. */
		{
			gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, m_bo);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");

			gl.compressedTextureSubImage3D(m_to_3D_compressed, 0, 0, 0, 0, s_reference_width, s_reference_height,
										   s_reference_depth, m_reference_compressed_3D_format,
										   m_reference_compressed_3D_size, (glw::GLubyte*)NULL + s_reference_size * 2);
			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glCompressedTextureSubImage3D",
									  "a non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target and "
									  "the buffer object's data store is currently mapped.");

			gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");
		}
	}

	return is_ok;
}

/** @brief Clean GL objects, test variables and GL errors.
 */
void SubImageErrorsTest::Clean()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Cleanup. */
	if (m_to_1D_empty)
	{
		gl.deleteTextures(1, &m_to_1D_empty);

		m_to_1D_empty = 0;
	}

	if (m_to_2D_empty)
	{
		gl.deleteTextures(1, &m_to_2D_empty);

		m_to_2D_empty = 0;
	}

	if (m_to_3D_empty)
	{
		gl.deleteTextures(1, &m_to_3D_empty);

		m_to_3D_empty = 0;
	}

	if (m_to_1D)
	{
		gl.deleteTextures(1, &m_to_1D);

		m_to_1D = 0;
	}

	if (m_to_2D)
	{
		gl.deleteTextures(1, &m_to_2D);

		m_to_2D = 0;
	}

	if (m_to_3D)
	{
		gl.deleteTextures(1, &m_to_3D);

		m_to_3D = 0;
	}

	if (m_to_1D_compressed)
	{
		gl.deleteTextures(1, &m_to_1D_compressed);

		m_to_1D_compressed = 0;
	}

	if (m_to_2D_compressed)
	{
		gl.deleteTextures(1, &m_to_2D_compressed);

		m_to_2D_compressed = 0;
	}

	if (m_to_3D_compressed)
	{
		gl.deleteTextures(1, &m_to_3D_compressed);

		m_to_3D_compressed = 0;
	}

	if (m_to_rectangle_compressed)
	{
		gl.deleteTextures(1, &m_to_rectangle_compressed);

		m_to_rectangle_compressed = 0;
	}

	if (m_bo)
	{
		gl.deleteBuffers(1, &m_bo);

		m_bo = 0;
	}

	m_to_invalid	   = 0;
	m_format_invalid   = 0;
	m_type_invalid	 = 0;
	m_max_texture_size = 1;

	if (DE_NULL != m_reference_compressed_1D)
	{
		delete[] m_reference_compressed_1D;

		m_reference_compressed_1D = NULL;
	}

	if (DE_NULL != m_reference_compressed_2D)
	{
		delete[] m_reference_compressed_2D;

		m_reference_compressed_2D = NULL;
	}

	if (DE_NULL != m_reference_compressed_3D)
	{
		delete[] m_reference_compressed_3D;

		m_reference_compressed_3D = NULL;
	}

	if (DE_NULL != m_reference_compressed_rectangle)
	{
		delete[] m_reference_compressed_rectangle;

		m_reference_compressed_rectangle = NULL;
	}

	m_reference_compressed_1D_format		= 0;
	m_reference_compressed_2D_format		= 0;
	m_reference_compressed_3D_format		= 0;
	m_reference_compressed_rectangle_format = 0;
	m_reference_compressed_1D_size			= 0;
	m_reference_compressed_2D_size			= 0;
	m_reference_compressed_3D_size			= 0;
	m_reference_compressed_rectangle_size   = 0;
	m_not_matching_compressed_1D_format		= 0;
	m_not_matching_compressed_1D_size		= 0;
	m_not_matching_compressed_2D_format		= 0;
	m_not_matching_compressed_2D_size		= 0;
	m_not_matching_compressed_3D_format		= 0;
	m_not_matching_compressed_3D_size		= 0;

	while (GL_NO_ERROR != gl.getError())
		;
}

/** Reference data */
const glw::GLushort SubImageErrorsTest::s_reference[] = {
	0x0,  0x0,  0x0,  0xff, 0x7f, 0x7f, 0x7f, 0xff, 0xc3, 0xc3, 0xc3, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x88, 0x0,  0x15, 0xff, 0xed, 0x1c, 0x24, 0xff, 0xff, 0x7f, 0x27, 0xff, 0xff, 0xf2, 0x0,  0xff,
	0xc8, 0xbf, 0xe7, 0xff, 0x70, 0x92, 0xbe, 0xff, 0x99, 0xd9, 0xea, 0xff, 0xb5, 0xe6, 0x1d, 0xff,
	0xa3, 0x49, 0xa4, 0xff, 0x3f, 0x48, 0xcc, 0xff, 0x0,  0xa2, 0xe8, 0xff, 0x22, 0xb1, 0x4c, 0xff,

	0x0,  0x0,  0x0,  0xff, 0x7f, 0x7f, 0x7f, 0xff, 0xc3, 0xc3, 0xc3, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x88, 0x0,  0x15, 0xff, 0xed, 0x1c, 0x24, 0xff, 0xff, 0x7f, 0x27, 0xff, 0xff, 0xf2, 0x0,  0xff,
	0xc8, 0xbf, 0xe7, 0xff, 0x70, 0x92, 0xbe, 0xff, 0x99, 0xd9, 0xea, 0xff, 0xb5, 0xe6, 0x1d, 0xff,
	0xa3, 0x49, 0xa4, 0xff, 0x3f, 0x48, 0xcc, 0xff, 0x0,  0xa2, 0xe8, 0xff, 0x22, 0xb1, 0x4c, 0xff,

	0x0,  0x0,  0x0,  0xff, 0x7f, 0x7f, 0x7f, 0xff, 0xc3, 0xc3, 0xc3, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x88, 0x0,  0x15, 0xff, 0xed, 0x1c, 0x24, 0xff, 0xff, 0x7f, 0x27, 0xff, 0xff, 0xf2, 0x0,  0xff,
	0xc8, 0xbf, 0xe7, 0xff, 0x70, 0x92, 0xbe, 0xff, 0x99, 0xd9, 0xea, 0xff, 0xb5, 0xe6, 0x1d, 0xff,
	0xa3, 0x49, 0xa4, 0xff, 0x3f, 0x48, 0xcc, 0xff, 0x0,  0xa2, 0xe8, 0xff, 0x22, 0xb1, 0x4c, 0xff,

	0x0,  0x0,  0x0,  0xff, 0x7f, 0x7f, 0x7f, 0xff, 0xc3, 0xc3, 0xc3, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x88, 0x0,  0x15, 0xff, 0xed, 0x1c, 0x24, 0xff, 0xff, 0x7f, 0x27, 0xff, 0xff, 0xf2, 0x0,  0xff,
	0xc8, 0xbf, 0xe7, 0xff, 0x70, 0x92, 0xbe, 0xff, 0x99, 0xd9, 0xea, 0xff, 0xb5, 0xe6, 0x1d, 0xff,
	0xa3, 0x49, 0xa4, 0xff, 0x3f, 0x48, 0xcc, 0xff, 0x0,  0xa2, 0xe8, 0xff, 0x22, 0xb1, 0x4c, 0xff
};

/** Reference data parameters. */
const glw::GLuint SubImageErrorsTest::s_reference_size						= sizeof(s_reference);
const glw::GLuint SubImageErrorsTest::s_reference_width						= 4;
const glw::GLuint SubImageErrorsTest::s_reference_height					= 4;
const glw::GLuint SubImageErrorsTest::s_reference_depth						= 4;
const glw::GLenum SubImageErrorsTest::s_reference_internalformat			= GL_RG8;
const glw::GLenum SubImageErrorsTest::s_reference_internalformat_compressed = GL_COMPRESSED_RG;
const glw::GLenum SubImageErrorsTest::s_reference_format = GL_RG; /* !Must not be a RGB, RGBA, or BGRA */
const glw::GLenum SubImageErrorsTest::s_reference_type   = GL_UNSIGNED_SHORT;

/******************************** Copy Errors Test Implementation   ********************************/

/** @brief Copy Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
CopyErrorsTest::CopyErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "textures_copy_errors", "Texture Copy Errors Test")
	, m_fbo(0)
	, m_fbo_ms(0)
	, m_fbo_incomplete(0)
	, m_to_src(0)
	, m_to_src_ms(0)
	, m_to_1D_dst(0)
	, m_to_2D_dst(0)
	, m_to_3D_dst(0)
	, m_to_invalid(0)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Copy Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult CopyErrorsTest::iterate()
{
	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		Prepare();

		is_ok &= Test1D();
		is_ok &= Test2D();
		is_ok &= Test3D();
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	Clean();

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** @brief Prepare test's objects and values.
 */
void CopyErrorsTest::Prepare()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Auxiliary objects setup. */

	/* Framebuffer. */
	gl.genFramebuffers(1, &m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	gl.createTextures(GL_TEXTURE_2D, 1, &m_to_src);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	gl.textureStorage2D(m_to_src, 1, s_internalformat, s_width, s_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTextureStorage2D has failed");

	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_to_src, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture1D call failed.");

	if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		throw 0;
	}

	gl.viewport(0, 0, s_width, s_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport call failed.");

	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear call failed.");

	/* Framebuffer Multisample. */
	gl.genFramebuffers(1, &m_fbo_ms);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_ms);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	gl.createTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &m_to_src_ms);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	gl.textureStorage2DMultisample(m_to_src_ms, 1, s_internalformat, s_width, s_height, false);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTextureStorage2DMultisample has failed");

	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_to_src_ms, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture1D call failed.");

	if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		throw 0;
	}

	gl.viewport(0, 0, s_width, s_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport call failed.");

	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear call failed.");

	/* Framebuffer Incomplete. */
	gl.createFramebuffers(1, &m_fbo_incomplete);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glcreateFramebuffers call failed.");

	/* 1D */
	gl.createTextures(GL_TEXTURE_1D, 1, &m_to_1D_dst);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	gl.textureStorage1D(m_to_1D_dst, 1, s_internalformat, s_width);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTextureStorage2D has failed");

	/* 2D */
	gl.createTextures(GL_TEXTURE_2D, 1, &m_to_2D_dst);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	gl.textureStorage2D(m_to_2D_dst, 1, s_internalformat, s_width, s_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTextureStorage2D has failed");

	/* 3D */
	gl.createTextures(GL_TEXTURE_3D, 1, &m_to_3D_dst);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	gl.textureStorage3D(m_to_3D_dst, 1, s_internalformat, s_width, s_height, s_depth);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTextureStorage2D has failed");

	/* invalid texture object */
	while (gl.isTexture(++m_to_invalid))
		;
	GLU_EXPECT_NO_ERROR(gl.getError(), "glIsTexture has failed");
}

/** @brief Test (negative) of CopyTextureSubImage1D
 *
 *  @return Test result.
 */
bool CopyErrorsTest::Test1D()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Result. */
	bool is_ok = true;

	/* Bind framebuffer. */
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_incomplete);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	/* Check that INVALID_FRAMEBUFFER_OPERATION is generated by
	 CopyTextureSubImage1D if the object bound to READ_FRAMEBUFFER_BINDING is
	 not framebuffer complete. */
	{
		gl.copyTextureSubImage1D(m_to_1D_dst, 0, 0, 0, 0, s_width);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_FRAMEBUFFER_OPERATION, "glCopyTextureSubImage1D",
								  "the object bound to READ_FRAMEBUFFER_BINDING is not framebuffer complete.");
	}

	/* Bind framebuffer. */
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	gl.readBuffer(GL_COLOR_ATTACHMENT0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	/* Check that INVALID_OPERATION is generated by CopyTextureSubImage1D if
	 texture is not the name of an existing texture object, or if the
	 effective target of texture is not TEXTURE_1D. */
	{
		gl.copyTextureSubImage1D(m_to_invalid, 0, 0, 0, 0, s_width);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glCopyTextureSubImage1D",
								  "texture is not the name of an existing texture object.");

		gl.copyTextureSubImage1D(m_to_2D_dst, 0, 0, 0, 0, s_width);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glCopyTextureSubImage1D",
								  "the effective target of texture is not TEXTURE_1D.");
	}

	/* Check that INVALID_VALUE is generated by CopyTextureSubImage1D if level is less than 0. */
	{
		gl.copyTextureSubImage1D(m_to_1D_dst, -1, 0, 0, 0, s_width);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glCopyTextureSubImage1D", "level is less than 0.");
	}

	/* Check that INVALID_VALUE is generated by CopyTextureSubImage1D if
	 xoffset<0, or (xoffset+width)>w, where w is the TEXTURE_WIDTH of the
	 texture image being modified. */
	{
		gl.copyTextureSubImage1D(m_to_1D_dst, 0, -1, 0, 0, s_width);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glCopyTextureSubImage1D", "xoffset<0.");

		gl.copyTextureSubImage1D(m_to_1D_dst, 0, 1, 0, 0, s_width);
		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glCopyTextureSubImage1D",
							 "(xoffset+width)>w, where w is the TEXTURE_WIDTH of the texture image being modified.");
	}

	/* Check that INVALID_OPERATION is generated by CopyTextureSubImage1D if
	 the read buffer is NONE. */
	gl.readBuffer(GL_NONE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	{
		gl.copyTextureSubImage1D(m_to_1D_dst, 0, 0, 0, 0, s_width);
		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glCopyTextureSubImage1D", "the read buffer is NONE.");
	}

	/* Bind multisample framebuffer. */
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_ms);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	gl.readBuffer(GL_COLOR_ATTACHMENT0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	/* Check that INVALID_OPERATION is generated by CopyTextureSubImage1D if
	 the effective value of SAMPLE_BUFFERS for the read
	 framebuffer is one. */
	{
		gl.copyTextureSubImage1D(m_to_1D_dst, 0, 0, 0, 0, s_width);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glCopyTextureSubImage1D",
								  "the effective value of SAMPLE_BUFFERS for the read framebuffer is one.");
	}

	return is_ok;
}

/** @brief Test (negative) of CopyTextureSubImage2D
 *
 *  @return Test result.
 */
bool CopyErrorsTest::Test2D()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Result. */
	bool is_ok = true;

	/* Bind framebuffer. */
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_incomplete);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	/* Check that INVALID_FRAMEBUFFER_OPERATION is generated by
	 CopyTextureSubImage2D if the object bound to READ_FRAMEBUFFER_BINDING is
	 not framebuffer complete. */
	{
		gl.copyTextureSubImage2D(m_to_2D_dst, 0, 0, 0, 0, 0, s_width, s_height);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_FRAMEBUFFER_OPERATION, "glCopyTextureSubImage2D",
								  "the object bound to READ_FRAMEBUFFER_BINDING is not framebuffer complete.");
	}

	/* Bind framebuffer. */
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	gl.readBuffer(GL_COLOR_ATTACHMENT0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	/* Check that INVALID_OPERATION is generated by CopyTextureSubImage2D if
	 texture is not the name of an existing texture object, or if the
	 effective target of texture is not TEXTURE_2D. */
	{
		gl.copyTextureSubImage2D(m_to_invalid, 0, 0, 0, 0, 0, s_width, s_height);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glCopyTextureSubImage2D",
								  "texture is not the name of an existing texture object.");

		gl.copyTextureSubImage2D(m_to_1D_dst, 0, 0, 0, 0, 0, s_width, s_height);
		is_ok &= CheckErrorAndLog(
			m_context, GL_INVALID_OPERATION, "glCopyTextureSubImage2D",
			"the effective target of does not correspond to one of the texture targets supported by the function..");
	}

	/* Check that INVALID_VALUE is generated by CopyTextureSubImage2D if level is less than 0. */
	{
		gl.copyTextureSubImage2D(m_to_2D_dst, -1, 0, 0, 0, 0, s_width, s_height);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glCopyTextureSubImage2D", "level is less than 0.");
	}

	/* Check that INVALID_VALUE is generated by CopyTextureSubImage2D if
	 xoffset<0, (xoffset+width)>w, yoffset<0, or (yoffset+height)>0, where w
	 is the TEXTURE_WIDTH, h is the TEXTURE_HEIGHT and of the texture image
	 being modified. */
	{
		gl.copyTextureSubImage2D(m_to_2D_dst, 0, -1, 0, 0, 0, s_width, s_height);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glCopyTextureSubImage2D", "xoffset<0.");

		gl.copyTextureSubImage2D(m_to_2D_dst, 0, 1, 0, 0, 0, s_width, s_height);
		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glCopyTextureSubImage2D",
							 "(xoffset+width)>w, where w is the TEXTURE_WIDTH of the texture image being modified.");

		gl.copyTextureSubImage2D(m_to_2D_dst, 0, 0, -1, 0, 0, s_width, s_height);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glCopyTextureSubImage2D", "yoffset<0.");

		gl.copyTextureSubImage2D(m_to_2D_dst, 0, 0, 1, 0, 0, s_width, s_height);
		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glCopyTextureSubImage2D",
							 "(yoffset+height)>h, where h is the TEXTURE_HEIGHT of the texture image being modified.");
	}

	/* Check that INVALID_OPERATION is generated by CopyTextureSubImage2D if
	 the read buffer is NONE. */
	gl.readBuffer(GL_NONE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	{
		gl.copyTextureSubImage2D(m_to_2D_dst, 0, 0, 0, 0, 0, s_width, s_height);
		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glCopyTextureSubImage2D", "the read buffer is NONE.");
	}

	/* Bind multisample framebuffer. */
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_ms);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	gl.readBuffer(GL_COLOR_ATTACHMENT0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	/* Check that INVALID_OPERATION is generated by CopyTextureSubImage2D if
	 the effective value of SAMPLE_BUFFERS for the read
	 framebuffer is one. */
	{
		gl.copyTextureSubImage2D(m_to_2D_dst, 0, 0, 0, 0, 0, s_width, s_height);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glCopyTextureSubImage2D",
								  "the effective value of SAMPLE_BUFFERS for the read framebuffer is one.");
	}

	return is_ok;
}

/** @brief Test (negative) of CopyTextureSubImage3D
 *
 *  @return Test result.
 */
bool CopyErrorsTest::Test3D()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Result. */
	bool is_ok = true;

	/* Bind framebuffer. */
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_incomplete);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	/* Check that INVALID_FRAMEBUFFER_OPERATION is generated by
	 CopyTextureSubImage3D if the object bound to READ_FRAMEBUFFER_BINDING is
	 not framebuffer complete. */
	{
		gl.copyTextureSubImage3D(m_to_3D_dst, 0, 0, 0, 0, 0, 0, s_width, s_height);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_FRAMEBUFFER_OPERATION, "glCopyTextureSubImage3D",
								  "the object bound to READ_FRAMEBUFFER_BINDING is not framebuffer complete.");
	}

	/* Bind framebuffer. */
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	gl.readBuffer(GL_COLOR_ATTACHMENT0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	/* Check that INVALID_OPERATION is generated by CopyTextureSubImage3D if
	 texture is not the name of an existing texture object, or if the
	 effective target of texture is not supported by the function. */
	{
		gl.copyTextureSubImage3D(m_to_invalid, 0, 0, 0, 0, 0, 0, s_width, s_height);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glCopyTextureSubImage3D",
								  "texture is not the name of an existing texture object.");

		gl.copyTextureSubImage3D(m_to_1D_dst, 0, 0, 0, 0, 0, 0, s_width, s_height);
		is_ok &= CheckErrorAndLog(
			m_context, GL_INVALID_OPERATION, "glCopyTextureSubImage3D",
			"the effective target of does not correspond to one of the texture targets supported by the function..");
	}

	/* Check that INVALID_VALUE is generated by CopyTextureSubImage3D if level is less than 0. */
	{
		gl.copyTextureSubImage3D(m_to_3D_dst, -1, 0, 0, 0, 0, 0, s_width, s_height);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glCopyTextureSubImage3D", "level is less than 0.");
	}

	/* Check that INVALID_VALUE is generated by CopyTextureSubImage3D if
	 xoffset<0, (xoffset+width)>w, yoffset<0, (yoffset+height)>h, zoffset<0,
	 or (zoffset+1)>d, where w is the TEXTURE_WIDTH, h is the TEXTURE_HEIGHT,
	 d is the TEXTURE_DEPTH and of the texture image being modified. Note
	 that w, h, and d include twice the border width.  */
	{
		gl.copyTextureSubImage3D(m_to_3D_dst, 0, -1, 0, 0, 0, 0, s_width, s_height);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glCopyTextureSubImage3D", "xoffset<0.");

		gl.copyTextureSubImage3D(m_to_3D_dst, 0, 1, 0, 0, 0, 0, s_width, s_height);
		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glCopyTextureSubImage3D",
							 "(xoffset+width)>w, where w is the TEXTURE_WIDTH of the texture image being modified.");

		gl.copyTextureSubImage3D(m_to_3D_dst, 0, 0, -1, 0, 0, 0, s_width, s_height);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glCopyTextureSubImage3D", "yoffset<0.");

		gl.copyTextureSubImage3D(m_to_3D_dst, 0, 0, 1, 0, 0, 0, s_width, s_height);
		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glCopyTextureSubImage3D",
							 "(yoffset+height)>h, where h is the TEXTURE_HEIGHT of the texture image being modified.");

		gl.copyTextureSubImage3D(m_to_3D_dst, 0, 0, 0, -1, 0, 0, s_width, s_height);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glCopyTextureSubImage3D", "zoffset<0.");

		gl.copyTextureSubImage3D(m_to_3D_dst, 0, 0, 0, s_depth + 1, 0, 0, s_width, s_height);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glCopyTextureSubImage3D",
								  "(zoffset+1)>d, where d is the TEXTURE_DEPTH of the texture image being modified.");
	}

	/* Check that INVALID_OPERATION is generated by CopyTextureSubImage3D if
	 the read buffer is NONE. */
	gl.readBuffer(GL_NONE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	{
		gl.copyTextureSubImage3D(m_to_3D_dst, 0, 0, 0, 0, 0, 0, s_width, s_height);
		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glCopyTextureSubImage3D", "the read buffer is NONE.");
	}

	/* Bind multisample framebuffer. */
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_ms);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	gl.readBuffer(GL_COLOR_ATTACHMENT0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	/* Check that INVALID_OPERATION is generated by CopyTextureSubImage3D if
	 the effective value of SAMPLE_BUFFERS for the read
	 framebuffer is one. */
	{
		gl.copyTextureSubImage3D(m_to_3D_dst, 0, 0, 0, 0, 0, 0, s_width, s_height);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glCopyTextureSubImage3D",
								  "the effective value of SAMPLE_BUFFERS for the read framebuffer is one.");
	}

	return is_ok;
}

/** @brief Clean GL objects, test variables and GL errors.
 */
void CopyErrorsTest::Clean()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Cleanup. */
	if (m_fbo)
	{
		gl.deleteFramebuffers(1, &m_fbo);

		m_fbo = 0;
	}

	if (m_fbo_ms)
	{
		gl.deleteFramebuffers(1, &m_fbo_ms);

		m_fbo_ms = 0;
	}

	if (m_fbo_incomplete)
	{
		gl.deleteFramebuffers(1, &m_fbo_incomplete);

		m_fbo_incomplete = 0;
	}

	if (m_to_src)
	{
		gl.deleteTextures(1, &m_to_src);

		m_to_src = 0;
	}

	if (m_to_src_ms)
	{
		gl.deleteTextures(1, &m_to_src_ms);

		m_to_src_ms = 0;
	}

	if (m_to_1D_dst)
	{
		gl.deleteTextures(1, &m_to_1D_dst);

		m_to_1D_dst = 0;
	}

	if (m_to_2D_dst)
	{
		gl.deleteTextures(1, &m_to_2D_dst);

		m_to_2D_dst = 0;
	}

	if (m_to_3D_dst)
	{
		gl.deleteTextures(1, &m_to_3D_dst);

		m_to_3D_dst = 0;
	}

	m_to_invalid = 0;

	while (GL_NO_ERROR != gl.getError())
		;
}

/* Test's parameters. */
const glw::GLuint CopyErrorsTest::s_width		   = 4;
const glw::GLuint CopyErrorsTest::s_height		   = 4;
const glw::GLuint CopyErrorsTest::s_depth		   = 4;
const glw::GLuint CopyErrorsTest::s_internalformat = GL_RGBA8;

/******************************** Parameter Setup Errors Test Implementation   ********************************/

/** @brief Parameter Setup Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
ParameterSetupErrorsTest::ParameterSetupErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "textures_parameter_setup_errors", "Texture Parameter Setup Errors Test")
	, m_to_2D(0)
	, m_to_2D_ms(0)
	, m_to_rectangle(0)
	, m_to_invalid(0)
	, m_pname_invalid(0)
	, m_depth_stencil_mode_invalid(0)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Parameter Setup Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult ParameterSetupErrorsTest::iterate()
{
	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		Prepare();

		is_ok &= Testf();
		is_ok &= Testi();
		is_ok &= Testfv();
		is_ok &= Testiv();
		is_ok &= TestIiv();
		is_ok &= TestIuiv();
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	Clean();

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** @brief Test's preparations.
 */
void ParameterSetupErrorsTest::Prepare()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Auxiliary objects setup. */

	/* 2D */
	gl.createTextures(GL_TEXTURE_2D, 1, &m_to_2D);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	/* 3D */
	gl.createTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &m_to_2D_ms);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	/* RECTANGLE */
	gl.createTextures(GL_TEXTURE_RECTANGLE, 1, &m_to_rectangle);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTextures has failed");

	/* Invalid texture object. */
	while (gl.isTexture(++m_to_invalid))
		;
	GLU_EXPECT_NO_ERROR(gl.getError(), "glIsTexture has failed");

	/* Invalid parameter name. */
	glw::GLenum all_pnames[] = { GL_DEPTH_STENCIL_TEXTURE_MODE,
								 GL_TEXTURE_BASE_LEVEL,
								 GL_TEXTURE_COMPARE_FUNC,
								 GL_TEXTURE_COMPARE_MODE,
								 GL_TEXTURE_LOD_BIAS,
								 GL_TEXTURE_MIN_FILTER,
								 GL_TEXTURE_MAG_FILTER,
								 GL_TEXTURE_MIN_LOD,
								 GL_TEXTURE_MAX_LOD,
								 GL_TEXTURE_MAX_LEVEL,
								 GL_TEXTURE_SWIZZLE_R,
								 GL_TEXTURE_SWIZZLE_G,
								 GL_TEXTURE_SWIZZLE_B,
								 GL_TEXTURE_SWIZZLE_A,
								 GL_TEXTURE_WRAP_S,
								 GL_TEXTURE_WRAP_T,
								 GL_TEXTURE_WRAP_R,
								 GL_TEXTURE_BORDER_COLOR,
								 GL_TEXTURE_SWIZZLE_RGBA };
	glw::GLuint all_pnames_count = sizeof(all_pnames) / sizeof(all_pnames[0]);

	bool is_valid = true;

	while (is_valid)
	{
		is_valid = false;
		++m_pname_invalid;

		for (glw::GLuint i = 0; i < all_pnames_count; ++i)
		{
			if (all_pnames[i] == m_pname_invalid)
			{
				is_valid = true;

				break;
			}
		}
	}

	/* Invalid depth stencil mode name. */
	glw::GLenum all_depth_stencil_modes[]	 = { GL_DEPTH_COMPONENT, GL_STENCIL_INDEX };
	glw::GLuint all_depth_stencil_modes_count = sizeof(all_depth_stencil_modes) / sizeof(all_depth_stencil_modes[0]);

	is_valid = true;

	while (is_valid)
	{
		is_valid = false;
		++m_depth_stencil_mode_invalid;

		for (glw::GLuint i = 0; i < all_depth_stencil_modes_count; ++i)
		{
			if (all_depth_stencil_modes[i] == m_depth_stencil_mode_invalid)
			{
				is_valid = true;

				break;
			}
		}
	}
}

/** @brief Test (negative) of TextureParameterf
 *
 *  @return Test result.
 */
bool ParameterSetupErrorsTest::Testf()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Result. */
	bool is_ok = true;

	/* Check that INVALID_ENUM is generated by TextureParameter* if pname is
	 not one of the accepted defined values. */
	{
		gl.textureParameterf(m_to_2D, m_pname_invalid, 1.f);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameterf",
								  "pname is not one of the accepted defined values.");
	}

	/* Check that INVALID_ENUM is generated by TextureParameter* if params
	 should have a defined constant value (based on the value of pname) and
	 does not. */
	{
		gl.textureParameterf(m_to_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, (glw::GLfloat)m_depth_stencil_mode_invalid);

		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameterf",
							 "params should have a defined constant value (based on the value of pname) and does not.");
	}
	/* Check that INVALID_ENUM is generated if TextureParameter{if} is called
	 for a non-scalar parameter (pname TEXTURE_BORDER_COLOR or
	 TEXTURE_SWIZZLE_RGBA). */
	{
		gl.textureParameterf(m_to_2D, GL_TEXTURE_BORDER_COLOR, 1.f);

		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameterf",
							 "called for a non-scalar parameter (pname TEXTURE_BORDER_COLOR or TEXTURE_SWIZZLE_RGBA).");
	}

	/* Check that INVALID_ENUM is generated by TextureParameter* if the
	 effective target is either TEXTURE_2D_MULTISAMPLE or
	 TEXTURE_2D_MULTISAMPLE_ARRAY, and pname is any of the sampler states. */
	{
		gl.textureParameterf(m_to_2D_ms, GL_TEXTURE_LOD_BIAS, 1.f);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameterf",
								  "the  effective target is either TEXTURE_2D_MULTISAMPLE or  "
								  "TEXTURE_2D_MULTISAMPLE_ARRAY, and pname is any of the sampler states.");
	}

	/* Check that INVALID_ENUM is generated by TextureParameter* if the
	 effective target is TEXTURE_RECTANGLE and either of pnames
	 TEXTURE_WRAP_S or TEXTURE_WRAP_T is set to either MIRROR_CLAMP_TO_EDGE,
	 MIRRORED_REPEAT or REPEAT. */
	{
		gl.textureParameterf(m_to_rectangle, GL_TEXTURE_WRAP_S, GL_MIRROR_CLAMP_TO_EDGE);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameterf",
								  "the effective target is TEXTURE_RECTANGLE and either of pnames TEXTURE_WRAP_S or "
								  "TEXTURE_WRAP_T is set to either MIRROR_CLAMP_TO_EDGE, MIRRORED_REPEAT or REPEAT.");
	}

	/* Check that INVALID_ENUM is generated by TextureParameter* if the
	 effective target is TEXTURE_RECTANGLE and pname TEXTURE_MIN_FILTER is
	 set to a value other than NEAREST or LINEAR (no mipmap filtering is
	 permitted). */
	{
		gl.textureParameterf(m_to_rectangle, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameterf",
								  "the effective target is TEXTURE_RECTANGLE and pname TEXTURE_MIN_FILTER is set to a "
								  "value other than NEAREST or LINEAR (no mipmap filtering is permitted).");
	}

	/* Check that INVALID_OPERATION is generated by TextureParameter* if the
	 effective target is either TEXTURE_2D_MULTISAMPLE or
	 TEXTURE_2D_MULTISAMPLE_ARRAY, and pname TEXTURE_BASE_LEVEL is set to a
	 value other than zero. */
	{
		gl.textureParameterf(m_to_2D_ms, GL_TEXTURE_BASE_LEVEL, 1.f);

		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureParameterf",
							 "the effective target is either TEXTURE_2D_MULTISAMPLE or TEXTURE_2D_MULTISAMPLE_ARRAY, "
							 "and pname TEXTURE_BASE_LEVEL is set to a value other than zero.");
	}

	/* Check that INVALID_OPERATION is generated by TextureParameter* if
	 texture is not the name of an existing texture object. */
	{
		gl.textureParameterf(m_to_invalid, GL_TEXTURE_LOD_BIAS, 1.f);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureParameterf",
								  "texture is not the name of an existing texture object.");
	}

	/* Check that INVALID_OPERATION is generated by TextureParameter* if the
	 effective target is TEXTURE_RECTANGLE and pname TEXTURE_BASE_LEVEL is
	 set to any value other than zero. */
	{
		gl.textureParameterf(m_to_rectangle, GL_TEXTURE_BASE_LEVEL, 1.f);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureParameterf",
								  "the effective target is TEXTURE_RECTANGLE and pname TEXTURE_BASE_LEVEL is set to "
								  "any value other than zero. ");
	}

	/* Check that INVALID_VALUE is generated by TextureParameter* if pname is
	 TEXTURE_BASE_LEVEL or TEXTURE_MAX_LEVEL, and param or params is
	 negative. */
	{
		gl.textureParameterf(m_to_2D, GL_TEXTURE_BASE_LEVEL, -1.f);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureParameterf",
								  "pname is TEXTURE_BASE_LEVEL and param is negative.");

		gl.textureParameterf(m_to_2D, GL_TEXTURE_MAX_LEVEL, -1.f);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureParameterf",
								  "pname is TEXTURE_MAX_LEVEL and param is negative.");
	}

	return is_ok;
}

/** @brief Test (negative) of TextureParameteri
 *
 *  @return Test result.
 */
bool ParameterSetupErrorsTest::Testi()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Result. */
	bool is_ok = true;

	/* Check that INVALID_ENUM is generated by TextureParameter* if pname is
	 not one of the accepted defined values. */
	{
		gl.textureParameteri(m_to_2D, m_pname_invalid, 1);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameteri",
								  "pname is not one of the accepted defined values.");
	}

	/* Check that INVALID_ENUM is generated by TextureParameter* if params
	 should have a defined constant value (based on the value of pname) and
	 does not. */
	{
		gl.textureParameteri(m_to_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, m_depth_stencil_mode_invalid);

		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameteri",
							 "params should have a defined constant value (based on the value of pname) and does not.");
	}
	/* Check that INVALID_ENUM is generated if TextureParameter{if} is called
	 for a non-scalar parameter (pname TEXTURE_BORDER_COLOR or
	 TEXTURE_SWIZZLE_RGBA). */
	{
		gl.textureParameteri(m_to_2D, GL_TEXTURE_BORDER_COLOR, 1);

		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameteri",
							 "called for a non-scalar parameter (pname TEXTURE_BORDER_COLOR or TEXTURE_SWIZZLE_RGBA).");
	}

	/* Check that INVALID_ENUM is generated by TextureParameter* if the
	 effective target is either TEXTURE_2D_MULTISAMPLE or
	 TEXTURE_2D_MULTISAMPLE_ARRAY, and pname is any of the sampler states. */
	{
		gl.textureParameteri(m_to_2D_ms, GL_TEXTURE_LOD_BIAS, 1);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameteri",
								  "the  effective target is either TEXTURE_2D_MULTISAMPLE or  "
								  "TEXTURE_2D_MULTISAMPLE_ARRAY, and pname is any of the sampler states.");
	}

	/* Check that INVALID_ENUM is generated by TextureParameter* if the
	 effective target is TEXTURE_RECTANGLE and either of pnames
	 TEXTURE_WRAP_S or TEXTURE_WRAP_T is set to either MIRROR_CLAMP_TO_EDGE,
	 MIRRORED_REPEAT or REPEAT. */
	{
		gl.textureParameteri(m_to_rectangle, GL_TEXTURE_WRAP_S, GL_MIRROR_CLAMP_TO_EDGE);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameteri",
								  "the effective target is TEXTURE_RECTANGLE and either of pnames TEXTURE_WRAP_S or "
								  "TEXTURE_WRAP_T is set to either MIRROR_CLAMP_TO_EDGE, MIRRORED_REPEAT or REPEAT.");
	}

	/* Check that INVALID_ENUM is generated by TextureParameter* if the
	 effective target is TEXTURE_RECTANGLE and pname TEXTURE_MIN_FILTER is
	 set to a value other than NEAREST or LINEAR (no mipmap filtering is
	 permitted). */
	{
		gl.textureParameteri(m_to_rectangle, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameteri",
								  "the effective target is TEXTURE_RECTANGLE and pname TEXTURE_MIN_FILTER is set to a "
								  "value other than NEAREST or LINEAR (no mipmap filtering is permitted).");
	}

	/* Check that INVALID_OPERATION is generated by TextureParameter* if the
	 effective target is either TEXTURE_2D_MULTISAMPLE or
	 TEXTURE_2D_MULTISAMPLE_ARRAY, and pname TEXTURE_BASE_LEVEL is set to a
	 value other than zero. */
	{
		gl.textureParameteri(m_to_2D_ms, GL_TEXTURE_BASE_LEVEL, 1);

		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureParameteri",
							 "the effective target is either TEXTURE_2D_MULTISAMPLE or TEXTURE_2D_MULTISAMPLE_ARRAY, "
							 "and pname TEXTURE_BASE_LEVEL is set to a value other than zero.");
	}

	/* Check that INVALID_OPERATION is generated by TextureParameter* if
	 texture is not the name of an existing texture object. */
	{
		gl.textureParameteri(m_to_invalid, GL_TEXTURE_LOD_BIAS, 1);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureParameteri",
								  "texture is not the name of an existing texture object.");
	}

	/* Check that INVALID_OPERATION is generated by TextureParameter* if the
	 effective target is TEXTURE_RECTANGLE and pname TEXTURE_BASE_LEVEL is
	 set to any value other than zero. */
	{
		gl.textureParameteri(m_to_rectangle, GL_TEXTURE_BASE_LEVEL, 1);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureParameteri",
								  "the effective target is TEXTURE_RECTANGLE and pname TEXTURE_BASE_LEVEL is set to "
								  "any value other than zero. ");
	}

	/* Check that INVALID_VALUE is generated by TextureParameter* if pname is
	 TEXTURE_BASE_LEVEL or TEXTURE_MAX_LEVEL, and param or params is
	 negative. */
	{
		gl.textureParameteri(m_to_2D, GL_TEXTURE_BASE_LEVEL, -1);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureParameteri",
								  "pname is TEXTURE_BASE_LEVEL and param is negative.");

		gl.textureParameteri(m_to_2D, GL_TEXTURE_MAX_LEVEL, -1);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureParameteri",
								  "pname is TEXTURE_MAX_LEVEL and param is negative.");
	}

	return is_ok;
}

/** @brief Test (negative) of TextureParameterfv
 *
 *  @return Test result.
 */
bool ParameterSetupErrorsTest::Testfv()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Result. */
	bool is_ok = true;

	glw::GLfloat one						= 1.f;
	glw::GLfloat minus_one					= -1.f;
	glw::GLfloat depth_stencil_mode_invalid = (glw::GLfloat)m_depth_stencil_mode_invalid;
	glw::GLfloat wrap_invalid				= (glw::GLfloat)GL_MIRROR_CLAMP_TO_EDGE;
	glw::GLfloat min_filter_invalid			= (glw::GLfloat)GL_NEAREST_MIPMAP_NEAREST;

	/* Check that INVALID_ENUM is generated by TextureParameter* if pname is
	 not one of the accepted defined values. */
	{
		gl.textureParameterfv(m_to_2D, m_pname_invalid, &one);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameterfv",
								  "pname is not one of the accepted defined values.");
	}

	/* Check that INVALID_ENUM is generated by TextureParameter* if params
	 should have a defined constant value (based on the value of pname) and
	 does not. */
	{
		gl.textureParameterfv(m_to_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, &depth_stencil_mode_invalid);

		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameterfv",
							 "params should have a defined constant value (based on the value of pname) and does not.");
	}

	/* Check that INVALID_ENUM is generated by TextureParameter* if the
	 effective target is either TEXTURE_2D_MULTISAMPLE or
	 TEXTURE_2D_MULTISAMPLE_ARRAY, and pname is any of the sampler states. */
	{
		gl.textureParameterfv(m_to_2D_ms, GL_TEXTURE_LOD_BIAS, &one);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameterfv",
								  "the  effective target is either TEXTURE_2D_MULTISAMPLE or  "
								  "TEXTURE_2D_MULTISAMPLE_ARRAY, and pname is any of the sampler states.");
	}

	/* Check that INVALID_ENUM is generated by TextureParameter* if the
	 effective target is TEXTURE_RECTANGLE and either of pnames
	 TEXTURE_WRAP_S or TEXTURE_WRAP_T is set to either MIRROR_CLAMP_TO_EDGE,
	 MIRRORED_REPEAT or REPEAT. */
	{
		gl.textureParameterfv(m_to_rectangle, GL_TEXTURE_WRAP_S, &wrap_invalid);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameterfv",
								  "the effective target is TEXTURE_RECTANGLE and either of pnames TEXTURE_WRAP_S or "
								  "TEXTURE_WRAP_T is set to either MIRROR_CLAMP_TO_EDGE, MIRRORED_REPEAT or REPEAT.");
	}

	/* Check that INVALID_ENUM is generated by TextureParameter* if the
	 effective target is TEXTURE_RECTANGLE and pname TEXTURE_MIN_FILTER is
	 set to a value other than NEAREST or LINEAR (no mipmap filtering is
	 permitted). */
	{
		gl.textureParameterfv(m_to_rectangle, GL_TEXTURE_MIN_FILTER, &min_filter_invalid);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameterfv",
								  "the effective target is TEXTURE_RECTANGLE and pname TEXTURE_MIN_FILTER is set to a "
								  "value other than NEAREST or LINEAR (no mipmap filtering is permitted).");
	}

	/* Check that INVALID_OPERATION is generated by TextureParameter* if the
	 effective target is either TEXTURE_2D_MULTISAMPLE or
	 TEXTURE_2D_MULTISAMPLE_ARRAY, and pname TEXTURE_BASE_LEVEL is set to a
	 value other than zero. */
	{
		gl.textureParameterfv(m_to_2D_ms, GL_TEXTURE_BASE_LEVEL, &one);

		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureParameterfv",
							 "the effective target is either TEXTURE_2D_MULTISAMPLE or TEXTURE_2D_MULTISAMPLE_ARRAY, "
							 "and pname TEXTURE_BASE_LEVEL is set to a value other than zero.");
	}

	/* Check that INVALID_OPERATION is generated by TextureParameter* if
	 texture is not the name of an existing texture object. */
	{
		gl.textureParameterfv(m_to_invalid, GL_TEXTURE_LOD_BIAS, &one);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureParameterfv",
								  "texture is not the name of an existing texture object.");
	}

	/* Check that INVALID_OPERATION is generated by TextureParameter* if the
	 effective target is TEXTURE_RECTANGLE and pname TEXTURE_BASE_LEVEL is
	 set to any value other than zero. */
	{
		gl.textureParameterfv(m_to_rectangle, GL_TEXTURE_BASE_LEVEL, &one);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureParameterfv",
								  "the effective target is TEXTURE_RECTANGLE and pname TEXTURE_BASE_LEVEL is set to "
								  "any value other than zero. ");
	}

	/* Check that INVALID_VALUE is generated by TextureParameter* if pname is
	 TEXTURE_BASE_LEVEL or TEXTURE_MAX_LEVEL, and param or params is
	 negative. */
	{
		gl.textureParameterfv(m_to_2D, GL_TEXTURE_BASE_LEVEL, &minus_one);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureParameterfv",
								  "pname is TEXTURE_BASE_LEVEL and param is negative.");

		gl.textureParameterfv(m_to_2D, GL_TEXTURE_MAX_LEVEL, &minus_one);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureParameterfv",
								  "pname is TEXTURE_MAX_LEVEL and param is negative.");
	}

	return is_ok;
}

/** @brief Test (negative) of TextureParameteriv
 *
 *  @return Test result.
 */
bool ParameterSetupErrorsTest::Testiv()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Result. */
	bool is_ok = true;

	glw::GLint one						  = 1;
	glw::GLint minus_one				  = -1;
	glw::GLint depth_stencil_mode_invalid = (glw::GLint)m_depth_stencil_mode_invalid;
	glw::GLint wrap_invalid				  = (glw::GLint)GL_MIRROR_CLAMP_TO_EDGE;
	glw::GLint min_filter_invalid		  = (glw::GLint)GL_NEAREST_MIPMAP_NEAREST;

	/* Check that INVALID_ENUM is generated by TextureParameter* if pname is
	 not one of the accepted defined values. */
	{
		gl.textureParameteriv(m_to_2D, m_pname_invalid, &one);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameteriv",
								  "pname is not one of the accepted defined values.");
	}

	/* Check that INVALID_ENUM is generated by TextureParameter* if params
	 should have a defined constant value (based on the value of pname) and
	 does not. */
	{
		gl.textureParameteriv(m_to_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, &depth_stencil_mode_invalid);

		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameteriv",
							 "params should have a defined constant value (based on the value of pname) and does not.");
	}

	/* Check that INVALID_ENUM is generated by TextureParameter* if the
	 effective target is either TEXTURE_2D_MULTISAMPLE or
	 TEXTURE_2D_MULTISAMPLE_ARRAY, and pname is any of the sampler states. */
	{
		gl.textureParameteriv(m_to_2D_ms, GL_TEXTURE_LOD_BIAS, &one);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameteriv",
								  "the  effective target is either TEXTURE_2D_MULTISAMPLE or  "
								  "TEXTURE_2D_MULTISAMPLE_ARRAY, and pname is any of the sampler states.");
	}

	/* Check that INVALID_ENUM is generated by TextureParameter* if the
	 effective target is TEXTURE_RECTANGLE and either of pnames
	 TEXTURE_WRAP_S or TEXTURE_WRAP_T is set to either MIRROR_CLAMP_TO_EDGE,
	 MIRRORED_REPEAT or REPEAT. */
	{
		gl.textureParameteriv(m_to_rectangle, GL_TEXTURE_WRAP_S, &wrap_invalid);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameteriv",
								  "the effective target is TEXTURE_RECTANGLE and either of pnames TEXTURE_WRAP_S or "
								  "TEXTURE_WRAP_T is set to either MIRROR_CLAMP_TO_EDGE, MIRRORED_REPEAT or REPEAT.");
	}

	/* Check that INVALID_ENUM is generated by TextureParameter* if the
	 effective target is TEXTURE_RECTANGLE and pname TEXTURE_MIN_FILTER is
	 set to a value other than NEAREST or LINEAR (no mipmap filtering is
	 permitted). */
	{
		gl.textureParameteriv(m_to_rectangle, GL_TEXTURE_MIN_FILTER, &min_filter_invalid);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameteriv",
								  "the effective target is TEXTURE_RECTANGLE and pname TEXTURE_MIN_FILTER is set to a "
								  "value other than NEAREST or LINEAR (no mipmap filtering is permitted).");
	}

	/* Check that INVALID_OPERATION is generated by TextureParameter* if the
	 effective target is either TEXTURE_2D_MULTISAMPLE or
	 TEXTURE_2D_MULTISAMPLE_ARRAY, and pname TEXTURE_BASE_LEVEL is set to a
	 value other than zero. */
	{
		gl.textureParameteriv(m_to_2D_ms, GL_TEXTURE_BASE_LEVEL, &one);

		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureParameteriv",
							 "the effective target is either TEXTURE_2D_MULTISAMPLE or TEXTURE_2D_MULTISAMPLE_ARRAY, "
							 "and pname TEXTURE_BASE_LEVEL is set to a value other than zero.");
	}

	/* Check that INVALID_OPERATION is generated by TextureParameter* if
	 texture is not the name of an existing texture object. */
	{
		gl.textureParameteriv(m_to_invalid, GL_TEXTURE_LOD_BIAS, &one);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureParameteriv",
								  "texture is not the name of an existing texture object.");
	}

	/* Check that INVALID_OPERATION is generated by TextureParameter* if the
	 effective target is TEXTURE_RECTANGLE and pname TEXTURE_BASE_LEVEL is
	 set to any value other than zero. */
	{
		gl.textureParameteriv(m_to_rectangle, GL_TEXTURE_BASE_LEVEL, &one);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureParameteriv",
								  "the effective target is TEXTURE_RECTANGLE and pname TEXTURE_BASE_LEVEL is set to "
								  "any value other than zero. ");
	}

	/* Check that INVALID_VALUE is generated by TextureParameter* if pname is
	 TEXTURE_BASE_LEVEL or TEXTURE_MAX_LEVEL, and param or params is
	 negative. */
	{
		gl.textureParameteriv(m_to_2D, GL_TEXTURE_BASE_LEVEL, &minus_one);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureParameteriv",
								  "pname is TEXTURE_BASE_LEVEL and param is negative.");

		gl.textureParameteriv(m_to_2D, GL_TEXTURE_MAX_LEVEL, &minus_one);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureParameteriv",
								  "pname is TEXTURE_MAX_LEVEL and param is negative.");
	}

	return is_ok;
}

/** @brief Test (negative) of TextureParameterIiv
 *
 *  @return Test result.
 */
bool ParameterSetupErrorsTest::TestIiv()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Result. */
	bool is_ok = true;

	glw::GLint one						  = 1;
	glw::GLint minus_one				  = -1;
	glw::GLint depth_stencil_mode_invalid = (glw::GLint)m_depth_stencil_mode_invalid;
	glw::GLint wrap_invalid				  = (glw::GLint)GL_MIRROR_CLAMP_TO_EDGE;
	glw::GLint min_filter_invalid		  = (glw::GLint)GL_NEAREST_MIPMAP_NEAREST;

	/* Check that INVALID_ENUM is generated by TextureParameter* if pname is
	 not one of the accepted defined values. */
	{
		gl.textureParameterIiv(m_to_2D, m_pname_invalid, &one);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameterIiv",
								  "pname is not one of the accepted defined values.");
	}

	/* Check that INVALID_ENUM is generated by TextureParameter* if params
	 should have a defined constant value (based on the value of pname) and
	 does not. */
	{
		gl.textureParameterIiv(m_to_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, &depth_stencil_mode_invalid);

		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameterIiv",
							 "params should have a defined constant value (based on the value of pname) and does not.");
	}

	/* Check that INVALID_ENUM is generated by TextureParameter* if the
	 effective target is either TEXTURE_2D_MULTISAMPLE or
	 TEXTURE_2D_MULTISAMPLE_ARRAY, and pname is any of the sampler states. */
	{
		gl.textureParameterIiv(m_to_2D_ms, GL_TEXTURE_LOD_BIAS, &one);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameterIiv",
								  "the  effective target is either TEXTURE_2D_MULTISAMPLE or  "
								  "TEXTURE_2D_MULTISAMPLE_ARRAY, and pname is any of the sampler states.");
	}

	/* Check that INVALID_ENUM is generated by TextureParameter* if the
	 effective target is TEXTURE_RECTANGLE and either of pnames
	 TEXTURE_WRAP_S or TEXTURE_WRAP_T is set to either MIRROR_CLAMP_TO_EDGE,
	 MIRRORED_REPEAT or REPEAT. */
	{
		gl.textureParameterIiv(m_to_rectangle, GL_TEXTURE_WRAP_S, &wrap_invalid);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameterIiv",
								  "the effective target is TEXTURE_RECTANGLE and either of pnames TEXTURE_WRAP_S or "
								  "TEXTURE_WRAP_T is set to either MIRROR_CLAMP_TO_EDGE, MIRRORED_REPEAT or REPEAT.");
	}

	/* Check that INVALID_ENUM is generated by TextureParameter* if the
	 effective target is TEXTURE_RECTANGLE and pname TEXTURE_MIN_FILTER is
	 set to a value other than NEAREST or LINEAR (no mipmap filtering is
	 permitted). */
	{
		gl.textureParameterIiv(m_to_rectangle, GL_TEXTURE_MIN_FILTER, &min_filter_invalid);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameterIiv",
								  "the effective target is TEXTURE_RECTANGLE and pname TEXTURE_MIN_FILTER is set to a "
								  "value other than NEAREST or LINEAR (no mipmap filtering is permitted).");
	}

	/* Check that INVALID_OPERATION is generated by TextureParameter* if the
	 effective target is either TEXTURE_2D_MULTISAMPLE or
	 TEXTURE_2D_MULTISAMPLE_ARRAY, and pname TEXTURE_BASE_LEVEL is set to a
	 value other than zero. */
	{
		gl.textureParameterIiv(m_to_2D_ms, GL_TEXTURE_BASE_LEVEL, &one);

		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureParameterIiv",
							 "the effective target is either TEXTURE_2D_MULTISAMPLE or TEXTURE_2D_MULTISAMPLE_ARRAY, "
							 "and pname TEXTURE_BASE_LEVEL is set to a value other than zero.");
	}

	/* Check that INVALID_OPERATION is generated by TextureParameter* if
	 texture is not the name of an existing texture object. */
	{
		gl.textureParameterIiv(m_to_invalid, GL_TEXTURE_LOD_BIAS, &one);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureParameterIiv",
								  "texture is not the name of an existing texture object.");
	}

	/* Check that INVALID_OPERATION is generated by TextureParameter* if the
	 effective target is TEXTURE_RECTANGLE and pname TEXTURE_BASE_LEVEL is
	 set to any value other than zero. */
	{
		gl.textureParameterIiv(m_to_rectangle, GL_TEXTURE_BASE_LEVEL, &one);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureParameterIiv",
								  "the effective target is TEXTURE_RECTANGLE and pname TEXTURE_BASE_LEVEL is set to "
								  "any value other than zero. ");
	}

	/* Check that INVALID_VALUE is generated by TextureParameter* if pname is
	 TEXTURE_BASE_LEVEL or TEXTURE_MAX_LEVEL, and param or params is
	 negative. */
	{
		gl.textureParameterIiv(m_to_2D, GL_TEXTURE_BASE_LEVEL, &minus_one);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureParameterIiv",
								  "pname is TEXTURE_BASE_LEVEL and param is negative.");

		gl.textureParameterIiv(m_to_2D, GL_TEXTURE_MAX_LEVEL, &minus_one);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glTextureParameterIiv",
								  "pname is TEXTURE_MAX_LEVEL and param is negative.");
	}

	return is_ok;
}

/** @brief Test (negative) of TextureParameterIuiv
 *
 *  @return Test result.
 */
bool ParameterSetupErrorsTest::TestIuiv()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Result. */
	bool is_ok = true;

	glw::GLuint one						   = 1;
	glw::GLuint depth_stencil_mode_invalid = (glw::GLint)m_depth_stencil_mode_invalid;
	glw::GLuint wrap_invalid			   = (glw::GLint)GL_MIRROR_CLAMP_TO_EDGE;
	glw::GLuint min_filter_invalid		   = (glw::GLint)GL_NEAREST_MIPMAP_NEAREST;

	/* Check that INVALID_ENUM is generated by TextureParameter* if pname is
	 not one of the accepted defined values. */
	{
		gl.textureParameterIuiv(m_to_2D, m_pname_invalid, &one);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameterIuiv",
								  "pname is not one of the accepted defined values.");
	}

	/* Check that INVALID_ENUM is generated by TextureParameter* if params
	 should have a defined constant value (based on the value of pname) and
	 does not. */
	{
		gl.textureParameterIuiv(m_to_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, &depth_stencil_mode_invalid);

		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameterIuiv",
							 "params should have a defined constant value (based on the value of pname) and does not.");
	}

	/* Check that INVALID_ENUM is generated by TextureParameter* if the
	 effective target is either TEXTURE_2D_MULTISAMPLE or
	 TEXTURE_2D_MULTISAMPLE_ARRAY, and pname is any of the sampler states. */
	{
		gl.textureParameterIuiv(m_to_2D_ms, GL_TEXTURE_LOD_BIAS, &one);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameterIuiv",
								  "the  effective target is either TEXTURE_2D_MULTISAMPLE or  "
								  "TEXTURE_2D_MULTISAMPLE_ARRAY, and pname is any of the sampler states.");
	}

	/* Check that INVALID_ENUM is generated by TextureParameter* if the
	 effective target is TEXTURE_RECTANGLE and either of pnames
	 TEXTURE_WRAP_S or TEXTURE_WRAP_T is set to either MIRROR_CLAMP_TO_EDGE,
	 MIRRORED_REPEAT or REPEAT. */
	{
		gl.textureParameterIuiv(m_to_rectangle, GL_TEXTURE_WRAP_S, &wrap_invalid);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameterIuiv",
								  "the effective target is TEXTURE_RECTANGLE and either of pnames TEXTURE_WRAP_S or "
								  "TEXTURE_WRAP_T is set to either MIRROR_CLAMP_TO_EDGE, MIRRORED_REPEAT or REPEAT.");
	}

	/* Check that INVALID_ENUM is generated by TextureParameter* if the
	 effective target is TEXTURE_RECTANGLE and pname TEXTURE_MIN_FILTER is
	 set to a value other than NEAREST or LINEAR (no mipmap filtering is
	 permitted). */
	{
		gl.textureParameterIuiv(m_to_rectangle, GL_TEXTURE_MIN_FILTER, &min_filter_invalid);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glTextureParameterIuiv",
								  "the effective target is TEXTURE_RECTANGLE and pname TEXTURE_MIN_FILTER is set to a "
								  "value other than NEAREST or LINEAR (no mipmap filtering is permitted).");
	}

	/* Check that INVALID_OPERATION is generated by TextureParameter* if the
	 effective target is either TEXTURE_2D_MULTISAMPLE or
	 TEXTURE_2D_MULTISAMPLE_ARRAY, and pname TEXTURE_BASE_LEVEL is set to a
	 value other than zero. */
	{
		gl.textureParameterIuiv(m_to_2D_ms, GL_TEXTURE_BASE_LEVEL, &one);

		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureParameterIuiv",
							 "the effective target is either TEXTURE_2D_MULTISAMPLE or TEXTURE_2D_MULTISAMPLE_ARRAY, "
							 "and pname TEXTURE_BASE_LEVEL is set to a value other than zero.");
	}

	/* Check that INVALID_OPERATION is generated by TextureParameter* if
	 texture is not the name of an existing texture object. */
	{
		gl.textureParameterIuiv(m_to_invalid, GL_TEXTURE_LOD_BIAS, &one);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureParameterIuiv",
								  "texture is not the name of an existing texture object.");
	}

	/* Check that INVALID_OPERATION is generated by TextureParameter* if the
	 effective target is TEXTURE_RECTANGLE and pname TEXTURE_BASE_LEVEL is
	 set to any value other than zero. */
	{
		gl.textureParameterIuiv(m_to_rectangle, GL_TEXTURE_BASE_LEVEL, &one);

		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glTextureParameterIuiv",
								  "the effective target is TEXTURE_RECTANGLE and pname TEXTURE_BASE_LEVEL is set to "
								  "any value other than zero. ");
	}

	return is_ok;
}

/** @brief Clean GL objects, test variables and GL errors.
 */
void ParameterSetupErrorsTest::Clean()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Cleanup. */
	if (m_to_2D)
	{
		gl.deleteTextures(1, &m_to_2D);

		m_to_2D = 0;
	}

	if (m_to_2D_ms)
	{
		gl.deleteTextures(1, &m_to_2D_ms);

		m_to_2D_ms = 0;
	}

	if (m_to_rectangle)
	{
		gl.deleteTextures(1, &m_to_rectangle);

		m_to_rectangle = 0;
	}

	if (m_to_invalid)
	{
		gl.deleteTextures(1, &m_to_invalid);

		m_to_invalid = 0;
	}

	m_to_invalid	= 0;
	m_pname_invalid = 0;

	while (GL_NO_ERROR != gl.getError())
		;
}

/******************************** Generate Mipmap Errors Test Implementation   ********************************/

/** @brief Generate Mipmap Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
GenerateMipmapErrorsTest::GenerateMipmapErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "textures_generate_mipmap_errors", "Texture Generate Mipmap Errors Test")
{
	/* Intentionally left blank. */
}

/** @brief Iterate Generate Mipmap Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult GenerateMipmapErrorsTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	/* Objects. */
	glw::GLuint texture_invalid = 0;
	glw::GLuint texture_cube	= 0;

	try
	{
		/* Preparations. */

		/* incomplete cube map */
		gl.genTextures(1, &texture_cube);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

		gl.bindTexture(GL_TEXTURE_CUBE_MAP, texture_cube);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

		gl.texImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, s_reference_internalformat, s_reference_width,
					  s_reference_height, 0, s_reference_format, s_reference_type, s_reference_data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage2D has failed");

		/* invalid texture */
		while (gl.isTexture(++texture_invalid))
			;

		/* Check that INVALID_OPERATION is generated by GenerateTextureMipmap if
		 texture is not the name of an existing texture object. */
		gl.generateTextureMipmap(texture_invalid);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glGenerateTextureMipmap",
								  "texture is not the name of an existing texture object.");

		/* Check that INVALID_OPERATION is generated by GenerateTextureMipmap if
		 target is TEXTURE_CUBE_MAP or TEXTURE_CUBE_MAP_ARRAY, and the specified
		 texture object is not cube complete or cube array complete,
		 respectively. */
		gl.generateTextureMipmap(texture_cube);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glGenerateTextureMipmap",
								  "target is TEXTURE_CUBE_MAP or TEXTURE_CUBE_MAP_ARRAY, and the specified texture "
								  "object is not cube complete or cube array complete, respectively.");
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	if (texture_cube)
	{
		gl.deleteTextures(1, &texture_cube);
	}

	while (GL_NO_ERROR != gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** Reference data. */
const glw::GLubyte GenerateMipmapErrorsTest::s_reference_data[] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
																	0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF };

/** Reference data parameters. */
const glw::GLuint GenerateMipmapErrorsTest::s_reference_width		   = 4;
const glw::GLuint GenerateMipmapErrorsTest::s_reference_height		   = 4;
const glw::GLenum GenerateMipmapErrorsTest::s_reference_internalformat = GL_R8;
const glw::GLenum GenerateMipmapErrorsTest::s_reference_format		   = GL_RED;
const glw::GLenum GenerateMipmapErrorsTest::s_reference_type		   = GL_UNSIGNED_BYTE;

/******************************** Bind Unit Errors Test Implementation   ********************************/

/** @brief Bind Unit Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
BindUnitErrorsTest::BindUnitErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "textures_bind_unit_errors", "Texture Bind Unit Errors Test")
{
	/* Intentionally left blank. */
}

/** @brief IterateBind Unit Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult BindUnitErrorsTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	/* Objects. */
	glw::GLuint texture_invalid = 0;

	try
	{
		/* Prepare invalid texture */
		while (gl.isTexture(++texture_invalid))
			;

		/* incomplete cube map */

		/* Check that INVALID_OPERATION error is generated if texture is not zero
		 or the name of an existing texture object. */
		gl.bindTextureUnit(0, texture_invalid);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glBindTextureUnit",
								  "texture is not zero or the name of an existing texture object.");
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	while (GL_NO_ERROR != gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/******************************** Image Query Errors Test Implementation   ********************************/

/** @brief Image Query Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
ImageQueryErrorsTest::ImageQueryErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "textures_image_query_errors", "Texture Image Query Errors Test")
{
	/* Intentionally left blank. */
}

/** Reference data. */
const glw::GLuint ImageQueryErrorsTest::s_reference_data[] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
															   0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF };

/** Reference data parameters. */
const glw::GLuint ImageQueryErrorsTest::s_reference_width					  = 4;
const glw::GLuint ImageQueryErrorsTest::s_reference_height					  = 4;
const glw::GLuint ImageQueryErrorsTest::s_reference_size					  = sizeof(s_reference_data);
const glw::GLenum ImageQueryErrorsTest::s_reference_internalformat			  = GL_R8;
const glw::GLenum ImageQueryErrorsTest::s_reference_internalformat_int		  = GL_R8I;
const glw::GLenum ImageQueryErrorsTest::s_reference_internalformat_compressed = GL_COMPRESSED_RED_RGTC1;
const glw::GLenum ImageQueryErrorsTest::s_reference_format					  = GL_RED;
const glw::GLenum ImageQueryErrorsTest::s_reference_type					  = GL_UNSIGNED_INT;

/** @brief Iterate Image Query Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult ImageQueryErrorsTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	/* Objects. */
	glw::GLuint buffer										  = 0;
	glw::GLuint texture_invalid								  = 0;
	glw::GLuint texture_2D									  = 0;
	glw::GLuint texture_2D_int								  = 0;
	glw::GLuint texture_2D_ms								  = 0;
	glw::GLuint texture_2D_stencil							  = 0;
	glw::GLuint texture_2D_compressed						  = 0;
	glw::GLuint texture_cube								  = 0;
	glw::GLuint texture_rectangle							  = 0;
	glw::GLint  max_level									  = 0;
	char		store[s_reference_size * 6 /* for cubemap */] = {};

	try
	{
		/* Preparations. */

		/* Buffer. */
		gl.createBuffers(1, &buffer);

		gl.namedBufferData(buffer, s_reference_size + 1, NULL, GL_STATIC_COPY);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBufferData has failed");

		/* 2D texture */
		gl.genTextures(1, &texture_2D);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

		gl.bindTexture(GL_TEXTURE_2D, texture_2D);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

		gl.texImage2D(GL_TEXTURE_2D, 0, s_reference_internalformat, s_reference_width, s_reference_height, 0,
					  s_reference_format, s_reference_type, s_reference_data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage2D has failed");

		/* 2D texture */
		gl.genTextures(1, &texture_2D);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

		gl.bindTexture(GL_TEXTURE_2D, texture_2D);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

		gl.texImage2D(GL_TEXTURE_2D, 0, s_reference_internalformat, s_reference_width, s_reference_height, 0,
					  s_reference_format, s_reference_type, s_reference_data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage2D has failed");

		/* incomplete cube map */
		gl.genTextures(1, &texture_cube);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

		gl.bindTexture(GL_TEXTURE_CUBE_MAP, texture_cube);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

		gl.texImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, s_reference_internalformat, s_reference_width,
					  s_reference_height, 0, s_reference_format, s_reference_type, s_reference_data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage2D has failed");

		/* 2D multisample */
		gl.createTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &texture_2D_ms);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

		gl.textureStorage2DMultisample(texture_2D_ms, 1, s_reference_internalformat, s_reference_width,
									   s_reference_height, false);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTextureStorage2DMultisample has failed");

		/* 2D stencil */
		gl.createTextures(GL_TEXTURE_2D, 1, &texture_2D_stencil);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

		gl.textureStorage2D(texture_2D_stencil, 1, GL_STENCIL_INDEX8, s_reference_width, s_reference_height);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTextureStorage2DMultisample has failed");

		/* 2D compressed texture  */
		gl.genTextures(1, &texture_2D_compressed);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

		gl.bindTexture(GL_TEXTURE_2D, texture_2D_compressed);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

		gl.texImage2D(GL_TEXTURE_2D, 0, s_reference_internalformat_compressed, s_reference_width, s_reference_height, 0,
					  s_reference_format, s_reference_type, s_reference_data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage2D has failed");

		gl.getIntegerv(GL_MAX_TEXTURE_SIZE, &max_level); /* assuming that x > log(x) */
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv has failed");

		/* Rectangle texture */
		gl.genTextures(1, &texture_rectangle);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

		gl.bindTexture(GL_TEXTURE_RECTANGLE, texture_rectangle);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

		gl.texImage2D(GL_TEXTURE_RECTANGLE, 0, s_reference_internalformat, s_reference_width, s_reference_height, 0,
					  s_reference_format, s_reference_type, s_reference_data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage2D has failed");

		/* invalid texture */
		while (gl.isTexture(++texture_invalid))
			;

		/* Tests. */

		/* Check that INVALID_ENUM is generated by GetTextureImage functions if
		 resulting texture target is not an accepted value TEXTURE_1D,
		 TEXTURE_2D, TEXTURE_3D, TEXTURE_1D_ARRAY, TEXTURE_2D_ARRAY,
		 TEXTURE_CUBE_MAP_ARRAY, TEXTURE_RECTANGLE, and TEXTURE_CUBE_MAP. */
		gl.getTextureImage(texture_2D_ms, 0, s_reference_format, s_reference_type, s_reference_size, store);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glGetTextureImage",
								  "resulting texture target is not an accepted value TEXTURE_1D, TEXTURE_2D, "
								  "TEXTURE_3D, TEXTURE_1D_ARRAY, TEXTURE_2D_ARRAY, TEXTURE_CUBE_MAP_ARRAY, "
								  "TEXTURE_RECTANGLE, and TEXTURE_CUBE_MAP.");

		/* Check that INVALID_OPERATION is generated by GetTextureImage
		 if texture is not the name of an existing texture object. */
		gl.getTextureImage(texture_invalid, 0, s_reference_format, s_reference_type, s_reference_size, store);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glGetTextureImage",
								  "texture is not the name of an existing texture object.");

		/* Check that INVALID_OPERATION error is generated by GetTextureImage if
		 the effective target is TEXTURE_CUBE_MAP or TEXTURE_CUBE_MAP_ARRAY, and
		 the texture object is not cube complete or cube array complete,
		 respectively. */
		gl.getTextureImage(texture_cube, 0, s_reference_format, s_reference_type, s_reference_size * 6, store);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glGetTextureImage",
								  "the effective target is TEXTURE_CUBE_MAP and the texture object is not cube "
								  "complete or cube array complete, respectively.");

		/* Check that GL_INVALID_VALUE is generated if level is less than 0 or
		 larger than the maximum allowable level. */
		gl.getTextureImage(texture_2D, -1, s_reference_format, s_reference_type, s_reference_size, store);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glGetTextureImage", "level is less than 0.");

		gl.getTextureImage(texture_2D, max_level, s_reference_format, s_reference_type, s_reference_size, store);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glGetTextureImage",
								  "level is larger than the maximum allowable level.");

		/* Check that INVALID_VALUE error is generated if level is non-zero and the
		 effective target is TEXTURE_RECTANGLE. */
		gl.getTextureImage(texture_rectangle, 1, s_reference_format, s_reference_type, s_reference_size, store);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glGetTextureImage",
								  "level is non-zero and the effective target is TEXTURE_RECTANGLE.");

		/* Check that INVALID_OPERATION error is generated if any of the following
		 mismatches between format and the internal format of the texture image
		 exist:
		 -  format is a color format (one of the formats in table 8.3 whose
		 target is the color buffer) and the base internal format of the
		 texture image is not a color format.
		 -  format is DEPTH_COMPONENT and the base internal format is  not
		 DEPTH_COMPONENT or DEPTH_STENCIL
		 -  format is DEPTH_STENCIL and the base internal format is not
		 DEPTH_STENCIL
		 -  format is STENCIL_INDEX and the base internal format is not
		 STENCIL_INDEX or DEPTH_STENCIL
		 -  format is one of the integer formats in table 8.3 and the internal
		 format of the texture image is not integer, or format is not one of
		 the integer formats in table 8.3 and the internal format is integer. */
		gl.getTextureImage(texture_2D_stencil, 0, s_reference_format /* red */, s_reference_type, s_reference_size,
						   store);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glGetTextureImage",
								  "format is a color format (one of the formats in table 8.3 whose target is the color "
								  "buffer) and the base internal format of the texture image is not a color format.");

		gl.getTextureImage(texture_2D, 0, GL_DEPTH_COMPONENT, s_reference_type, s_reference_size, store);
		is_ok &= CheckErrorAndLog(
			m_context, GL_INVALID_OPERATION, "glGetTextureImage",
			"format is DEPTH_COMPONENT and the base internal format is not DEPTH_COMPONENT or DEPTH_STENCIL.");

		gl.getTextureImage(texture_2D, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, s_reference_size, store);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glGetTextureImage",
								  "format is DEPTH_STENCIL and the base internal format is not DEPTH_STENCIL.");

		gl.getTextureImage(texture_2D, 0, GL_STENCIL_INDEX, s_reference_type, s_reference_size, store);
		is_ok &= CheckErrorAndLog(
			m_context, GL_INVALID_OPERATION, "glGetTextureImage",
			"format is STENCIL_INDEX and the base internal format is not STENCIL_INDEX or DEPTH_STENCIL.");

		gl.getTextureImage(texture_2D, 0, GL_RED_INTEGER, s_reference_type, s_reference_size, store);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glGetTextureImage",
								  "format is one of the integer formats in table 8.3 and the internal format of the "
								  "texture image is not integer.");

		gl.getTextureImage(texture_2D_int, 0, GL_RED, s_reference_type, s_reference_size, store);
		is_ok &= CheckErrorAndLog(
			m_context, GL_INVALID_OPERATION, "glGetTextureImage",
			"format is not one of the integer formats in table 8.3 and the internal format is integer.");

		/* Check that INVALID_OPERATION error is generated if a pixel pack buffer
		 object is bound and packing the texture image into the buffer’s memory
		 would exceed the size of the buffer. */
		gl.bindBuffer(GL_PIXEL_PACK_BUFFER, buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");

		gl.getTextureImage(texture_2D, 0, s_reference_format, s_reference_type, s_reference_size,
						   (glw::GLuint*)NULL + 1);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glGetTextureImage",
								  "a pixel pack buffer object is bound and packing the texture image into the buffer’s "
								  "memory would exceed the size of the buffer.");

		gl.bindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");

		/* Check that INVALID_OPERATION error is generated if a pixel pack buffer
		 object is bound and pixels is not evenly divisible by the number of
		 basic machine units needed to store in memory the GL data type
		 corresponding to type (see table 8.2). */
		gl.bindBuffer(GL_PIXEL_PACK_BUFFER, buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");

		gl.getTextureImage(texture_2D, 0, s_reference_format, s_reference_type, s_reference_size,
						   (glw::GLubyte*)NULL + 1);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glGetTextureImage",
								  "a pixel pack buffer object is bound and pixels is not evenly divisible by the "
								  "number of basic machine units needed to store in memory the GL data type "
								  "corresponding to type (see table 8.2).");

		gl.bindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");

		/* Check that INVALID_OPERATION error is generated by GetTextureImage if
		 the buffer size required to store the requested data is greater than
		 bufSize. */
		gl.getTextureImage(texture_2D, 0, s_reference_format, s_reference_type,
						   s_reference_size - sizeof(s_reference_data[0]), store);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glGetTextureImage",
								  "the buffer size required to store the requested data is greater than bufSize.");

		/* Check that INVALID_OPERATION is generated by GetCompressedTextureImage
		 if texture is not the name of an existing texture object. */
		gl.getCompressedTextureImage(texture_invalid, 0, s_reference_size, store);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glGetCompressedTextureImage",
								  "texture is not the name of an existing texture object.");

		/* Check that INVALID_VALUE is generated by GetCompressedTextureImage if
		 level is less than zero or greater than the maximum number of LODs
		 permitted by the implementation. */
		gl.getCompressedTextureImage(texture_2D_compressed, -1, s_reference_size, store);
		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glGetCompressedTextureImage", "level is less than zero.");

		gl.getCompressedTextureImage(texture_2D_compressed, max_level, s_reference_size, store);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glGetCompressedTextureImage",
								  "level is greater than the maximum number of LODs permitted by the implementation.");

		/* Check that INVALID_OPERATION is generated if GetCompressedTextureImage
		 is used to retrieve a texture that is in an uncompressed internal
		 format. */
		gl.getCompressedTextureImage(texture_2D, 0, s_reference_size, store);
		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glGetCompressedTextureImage",
							 "the function is used to retrieve a texture that is in an uncompressed internal format.");

		/* Check that INVALID_OPERATION is generated by GetCompressedTextureImage
		 if a non-zero buffer object name is bound to the PIXEL_PACK_BUFFER
		 target, the buffer storage was not initialized with BufferStorage using
		 MAP_PERSISTENT_BIT flag, and the buffer object's data store is currently
		 mapped. */
		gl.bindBuffer(GL_PIXEL_PACK_BUFFER, buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");

		gl.mapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_WRITE);

		if (GL_NO_ERROR == gl.getError())
		{
			gl.getCompressedTextureImage(texture_2D_compressed, 0, s_reference_size, NULL);
			is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glGetCompressedTextureImage",
									  "a non-zero buffer object name is bound to the PIXEL_PACK_BUFFER target, the "
									  "buffer storage was not initialized with BufferStorage using MAP_PERSISTENT_BIT "
									  "flag, and the buffer object's data store is currently mapped.");

			gl.unmapBuffer(GL_PIXEL_PACK_BUFFER);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer has failed");
		}
		else
		{
			throw 0;
		}

		gl.bindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");

		/* Check that INVALID_OPERATION is generated by GetCompressedTextureImage
		 if a non-zero buffer object name is bound to the PIXEL_PACK_BUFFER
		 target and the data would be packed to the buffer object such that the
		 memory writes required would exceed the data store size. */
		gl.bindBuffer(GL_PIXEL_PACK_BUFFER, buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");

		gl.getCompressedTextureImage(texture_2D_compressed, 0, s_reference_size, (char*)NULL + s_reference_size - 1);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glGetCompressedTextureImage",
								  "a non-zero buffer object name is bound to the PIXEL_PACK_BUFFER target and the data "
								  "would be packed to the buffer object such that the memory writes required would "
								  "exceed the data store size.");

		gl.bindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer has failed");
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	if (buffer)
	{
		gl.deleteBuffers(1, &buffer);
	}

	if (texture_2D)
	{
		gl.deleteTextures(1, &texture_2D);
	}

	if (texture_2D_int)
	{
		gl.deleteTextures(1, &texture_2D_int);
	}

	if (texture_2D_stencil)
	{
		gl.deleteTextures(1, &texture_2D_stencil);
	}

	if (texture_2D_ms)
	{
		gl.deleteTextures(1, &texture_2D_ms);
	}

	if (texture_2D_compressed)
	{
		gl.deleteTextures(1, &texture_2D_compressed);
	}

	if (texture_cube)
	{
		gl.deleteTextures(1, &texture_cube);
	}

	if (texture_rectangle)
	{
		gl.deleteTextures(1, &texture_rectangle);
	}

	while (GL_NO_ERROR != gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/******************************** Level Parameter Query Errors Test Implementation   ********************************/

/** @brief Image Query Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
LevelParameterErrorsTest::LevelParameterErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "textures_level_parameter_errors", "Texture Level Parameter Query Errors Test")
{
	/* Intentionally left blank. */
}

/** @brief Iterate Level Parameter Query Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult LevelParameterErrorsTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	/* Objects. */
	glw::GLuint texture_2D		= 0;
	glw::GLuint texture_invalid = 0;
	glw::GLint  max_level		= 0;
	glw::GLenum pname_invalid   = 0;

	glw::GLfloat storef[4] = {};
	glw::GLint   storei[4] = {};

	try
	{
		/* Preparations. */

		/* 2D texture */
		gl.genTextures(1, &texture_2D);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

		gl.bindTexture(GL_TEXTURE_2D, texture_2D);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

		gl.texStorage2D(GL_TEXTURE_2D, 1, GL_R8, 1, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage2D has failed");

		/* Limits. */
		gl.getIntegerv(GL_MAX_TEXTURE_SIZE, &max_level); /* assuming that x > log(x) */
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv has failed");

		/* invalid texture */
		while (gl.isTexture(++texture_invalid))
			;

		/* invalid pname */
		glw::GLenum all_pnames[] = { GL_TEXTURE_WIDTH,
									 GL_TEXTURE_HEIGHT,
									 GL_TEXTURE_DEPTH,
									 GL_TEXTURE_SAMPLES,
									 GL_TEXTURE_FIXED_SAMPLE_LOCATIONS,
									 GL_TEXTURE_INTERNAL_FORMAT,
									 GL_TEXTURE_RED_SIZE,
									 GL_TEXTURE_GREEN_SIZE,
									 GL_TEXTURE_BLUE_SIZE,
									 GL_TEXTURE_ALPHA_SIZE,
									 GL_TEXTURE_DEPTH_SIZE,
									 GL_TEXTURE_STENCIL_SIZE,
									 GL_TEXTURE_SHARED_SIZE,
									 GL_TEXTURE_RED_TYPE,
									 GL_TEXTURE_GREEN_TYPE,
									 GL_TEXTURE_BLUE_TYPE,
									 GL_TEXTURE_ALPHA_TYPE,
									 GL_TEXTURE_DEPTH_TYPE,
									 GL_TEXTURE_COMPRESSED,
									 GL_TEXTURE_COMPRESSED_IMAGE_SIZE,
									 GL_TEXTURE_BUFFER_DATA_STORE_BINDING,
									 GL_TEXTURE_BUFFER_OFFSET,
									 GL_TEXTURE_BUFFER_SIZE };

		glw::GLuint all_pnames_count = sizeof(all_pnames) / sizeof(all_pnames[0]);

		bool is_valid = true;

		while (is_valid)
		{
			is_valid = false;

			++pname_invalid;

			for (glw::GLuint i = 0; i < all_pnames_count; ++i)
			{
				if (all_pnames[i] == pname_invalid)
				{
					is_valid = true;

					break;
				}
			}
		}

		/* Tests. */

		/* Check that INVALID_OPERATION is generated by GetTextureLevelParameterfv
		 and GetTextureLevelParameteriv functions if texture is not the name of
		 an existing texture object. */
		gl.getTextureLevelParameterfv(texture_invalid, 0, GL_TEXTURE_WIDTH, storef);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glGetTextureLevelParameterfv",
								  "texture is not the name of an existing texture object.");

		gl.getTextureLevelParameteriv(texture_invalid, 0, GL_TEXTURE_WIDTH, storei);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glGetTextureLevelParameteriv",
								  "texture is not the name of an existing texture object.");

		/* Check that INVALID_VALUE is generated by GetTextureLevelParameter* if
		 level is less than 0. */
		gl.getTextureLevelParameterfv(texture_2D, -1, GL_TEXTURE_WIDTH, storef);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glGetTextureLevelParameterfv", "level is less than 0.");

		gl.getTextureLevelParameteriv(texture_2D, -1, GL_TEXTURE_WIDTH, storei);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glGetTextureLevelParameteriv", "level is less than 0.");

		/* Check that INVALID_ENUM error is generated by GetTextureLevelParameter*
		 if pname is not one of supported constants. */
		gl.getTextureLevelParameterfv(texture_2D, 0, pname_invalid, storef);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glGetTextureLevelParameterfv",
								  "pname is not one of supported constants.");

		gl.getTextureLevelParameteriv(texture_2D, 0, pname_invalid, storei);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glGetTextureLevelParameteriv",
								  "pname is not one of supported constants.");

		/* Check that INVALID_VALUE may be generated if level is greater than
		 log2 max, where max is the returned value of MAX_TEXTURE_SIZE. */
		gl.getTextureLevelParameterfv(texture_2D, max_level, GL_TEXTURE_WIDTH, storef);
		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glGetTextureLevelParameterfv",
							 "level is greater than log2 max, where max is the returned value of MAX_TEXTURE_SIZE.");

		gl.getTextureLevelParameteriv(texture_2D, max_level, GL_TEXTURE_WIDTH, storei);
		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_VALUE, "glGetTextureLevelParameteriv",
							 "level is greater than log2 max, where max is the returned value of MAX_TEXTURE_SIZE.");

		/* Check that INVALID_OPERATION is generated by GetTextureLevelParameter*
		 if TEXTURE_COMPRESSED_IMAGE_SIZE is queried on texture images with an
		 uncompressed internal format or on proxy targets. */
		gl.getTextureLevelParameterfv(texture_2D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, storef);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glGetTextureLevelParameterfv",
								  "TEXTURE_COMPRESSED_IMAGE_SIZE is queried on texture images with an uncompressed "
								  "internal format or on proxy targets.");

		gl.getTextureLevelParameteriv(texture_2D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, storei);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glGetTextureLevelParameteriv",
								  "TEXTURE_COMPRESSED_IMAGE_SIZE is queried on texture images with an uncompressed "
								  "internal format or on proxy targets.");
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	if (texture_2D)
	{
		gl.deleteTextures(1, &texture_2D);
	}

	while (GL_NO_ERROR != gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/******************************** Parameter Query Errors Test Implementation   ********************************/

/** @brief Parameter Query Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
ParameterErrorsTest::ParameterErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "textures_parameter_errors", "Texture Parameter Query Errors Test")
{
	/* Intentionally left blank. */
}

/** @brief Iterate Parameter Query Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult ParameterErrorsTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	/* Objects. */
	glw::GLuint texture_2D		= 0;
	glw::GLuint texture_buffer  = 0;
	glw::GLuint texture_invalid = 0;
	glw::GLenum pname_invalid   = 0;

	glw::GLfloat storef[4] = {};
	glw::GLint   storei[4] = {};
	glw::GLuint  storeu[4] = {};

	try
	{
		/* Preparations. */

		/* 2D texture */
		gl.createTextures(GL_TEXTURE_2D, 1, &texture_2D);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

		/* Buffer texture */
		gl.createTextures(GL_TEXTURE_BUFFER, 1, &texture_buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

		/* invalid texture */
		while (gl.isTexture(++texture_invalid))
			;

		/* invalid pname */
		glw::GLenum all_pnames[] = { GL_IMAGE_FORMAT_COMPATIBILITY_TYPE,
									 GL_TEXTURE_IMMUTABLE_FORMAT,
									 GL_TEXTURE_IMMUTABLE_LEVELS,
									 GL_TEXTURE_TARGET,
									 GL_TEXTURE_VIEW_MIN_LEVEL,
									 GL_TEXTURE_VIEW_NUM_LEVELS,
									 GL_TEXTURE_VIEW_MIN_LAYER,
									 GL_TEXTURE_VIEW_NUM_LAYERS,
									 GL_DEPTH_STENCIL_TEXTURE_MODE,
									 GL_DEPTH_COMPONENT,
									 GL_STENCIL_INDEX,
									 GL_TEXTURE_BASE_LEVEL,
									 GL_TEXTURE_BORDER_COLOR,
									 GL_TEXTURE_COMPARE_MODE,
									 GL_TEXTURE_COMPARE_FUNC,
									 GL_TEXTURE_LOD_BIAS,
									 GL_TEXTURE_MAG_FILTER,
									 GL_TEXTURE_MAX_LEVEL,
									 GL_TEXTURE_MAX_LOD,
									 GL_TEXTURE_MIN_FILTER,
									 GL_TEXTURE_MIN_LOD,
									 GL_TEXTURE_SWIZZLE_R,
									 GL_TEXTURE_SWIZZLE_G,
									 GL_TEXTURE_SWIZZLE_B,
									 GL_TEXTURE_SWIZZLE_A,
									 GL_TEXTURE_SWIZZLE_RGBA,
									 GL_TEXTURE_WRAP_S,
									 GL_TEXTURE_WRAP_T,
									 GL_TEXTURE_WRAP_R };

		glw::GLuint all_pnames_count = sizeof(all_pnames) / sizeof(all_pnames[0]);

		bool is_valid = true;

		while (is_valid)
		{
			is_valid = false;

			++pname_invalid;

			for (glw::GLuint i = 0; i < all_pnames_count; ++i)
			{
				if (all_pnames[i] == pname_invalid)
				{
					is_valid = true;

					break;
				}
			}
		}

		/* Tests. */

		/* Check that INVALID_ENUM is generated by glGetTextureParameter* if pname
		 is not an accepted value. */
		gl.getTextureParameterfv(texture_2D, pname_invalid, storef);
		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glGetTextureParameterfv", "pname is not an accepted value.");

		gl.getTextureParameterIiv(texture_2D, pname_invalid, storei);
		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glGetTextureParameterIiv", "pname is not an accepted value.");

		gl.getTextureParameterIuiv(texture_2D, pname_invalid, storeu);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glGetTextureParameterIuiv",
								  "pname is not an accepted value.");

		gl.getTextureParameteriv(texture_2D, pname_invalid, storei);
		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glGetTextureParameteriv", "pname is not an accepted value.");

		/* Check that INVALID_OPERATION is generated by glGetTextureParameter* if
		 texture is not the name of an existing texture object. */
		gl.getTextureParameterfv(texture_invalid, GL_TEXTURE_TARGET, storef);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glGetTextureParameterfv",
								  "texture is not the name of an existing texture object.");

		gl.getTextureParameterIiv(texture_invalid, GL_TEXTURE_TARGET, storei);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glGetTextureParameterIiv",
								  "texture is not the name of an existing texture object.");

		gl.getTextureParameterIuiv(texture_invalid, GL_TEXTURE_TARGET, storeu);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glGetTextureParameterIuiv",
								  "texture is not the name of an existing texture object.");

		gl.getTextureParameteriv(texture_invalid, GL_TEXTURE_TARGET, storei);
		is_ok &= CheckErrorAndLog(m_context, GL_INVALID_OPERATION, "glGetTextureParameteriv",
								  "texture is not the name of an existing texture object.");

		/* Check that INVALID_ENUM error is generated if the effective target is
		 not one of the supported texture targets (eg. TEXTURE_BUFFER). */
		gl.getTextureParameterfv(texture_buffer, GL_TEXTURE_TARGET, storef);
		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glGetTextureParameterfv",
							 "the effective target is not one of the supported texture targets (eg. TEXTURE_BUFFER).");

		gl.getTextureParameterIiv(texture_buffer, GL_TEXTURE_TARGET, storei);
		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glGetTextureParameterIiv",
							 "the effective target is not one of the supported texture targets (eg. TEXTURE_BUFFER).");

		gl.getTextureParameterIuiv(texture_buffer, GL_TEXTURE_TARGET, storeu);
		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glGetTextureParameterIuiv",
							 "the effective target is not one of the supported texture targets (eg. TEXTURE_BUFFER).");

		gl.getTextureParameteriv(texture_buffer, GL_TEXTURE_TARGET, storei);
		is_ok &=
			CheckErrorAndLog(m_context, GL_INVALID_ENUM, "glGetTextureParameteriv",
							 "the effective target is not one of the supported texture targets (eg. TEXTURE_BUFFER).");
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	if (texture_2D)
	{
		gl.deleteTextures(1, &texture_2D);
	}

	if (texture_buffer)
	{
		gl.deleteTextures(1, &texture_buffer);
	}

	while (GL_NO_ERROR != gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

} /* Textures namespace. */
} /* DirectStateAccess namespace. */
} /* gl4cts namespace. */
