#pragma once
#if COMPILE_VULKAN

IGNORE_WARNINGS_PUSH
#include <LinearMath\btIDebugDraw.h>
IGNORE_WARNINGS_POP

#include <vector>

#include "Graphics/VertexBufferData.hpp"
#include "Transform.hpp"
#include "Types.hpp"

namespace flex
{
	namespace vk
	{
		class VulkanRenderer;

		class VulkanPhysicsDebugDraw : public btIDebugDraw
		{
		public:
			VulkanPhysicsDebugDraw();
			virtual ~VulkanPhysicsDebugDraw();

			void Initialize();

			void UpdateDebugMode();

			virtual void reportErrorWarning(const char* warningString)  override;
			virtual void draw3dText(const btVector3& location, const char* textString)  override;
			virtual void setDebugMode(int debugMode)  override;
			virtual int	getDebugMode() const override;

			virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override;
			virtual void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override;

			virtual void flushLines() override;
			void ClearLines();

		private:
			void Draw();

			struct LineSegment
			{
				btVector3 start;
				btVector3 end;
				btVector3 color;
			};

			// Gets filled each frame by calls to drawLine, then emptied after debugDrawWorld()
			std::vector<LineSegment> m_LineSegments;

			int m_DebugMode = 0;

			VulkanRenderer* m_Renderer = nullptr;

			MaterialID m_MaterialID = InvalidMaterialID;

			// Per-frame data
			VertexBufferData m_VertexBufferData;

			GameObject* m_GameObject = nullptr;
		};
	} // namespace vk
} // namespace flex

#endif // COMPILE_VULKAN