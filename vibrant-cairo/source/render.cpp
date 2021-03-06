#include "pch.hpp"

#include "vibrant/cairo/render.hpp"

#include "vibrant/renderable.hpp"
#include "vibrant/body.hpp"

namespace vibrant
{
using std::get;

class render_visitor : public boost::static_visitor<>
{
 public:
  render_visitor(cairo_t* context, Body::Handle body) : context(context), body(body) {}

  void operator()(Line line) const
  {
    cairo_save(context);

    cairo_translate(context, body->position.x, body->position.y);
    cairo_rotate(context, body->rotation);
    cairo_move_to(context, 0, 0);

    double line_length = std::max(body->size.x, body->size.y);

    cairo_line_to(context, line_length, 0);

    Rgb color = line.stroke.color;
    // TODO: Using rgba on PDF/Postfix/Print might degrade to bitmaps even if alpha is fully opaque.
    //       Confirm if this is true or not.
    cairo_set_source_rgba(context, color.r, color.g, color.b, color.a);
    cairo_set_line_width(context, line.stroke.width);
    cairo_stroke(context);

    cairo_restore(context);
  }

  void operator()(Rectangle rect) const
  {
    cairo_save(context);

    cairo_translate(context, body->position.x, body->position.y);
    cairo_rotate(context, body->rotation);
    cairo_rectangle(context, -body->size.x / 2, -body->size.y / 2, body->size.x, body->size.y);

    Rgb fill_color = rect.fill.color;
    // TODO: Using rgba on PDF/Postfix/Print might degrade to bitmaps even if alpha is fully opaque.
    //       Confirm if this is true or not.
    cairo_set_source_rgba(context, fill_color.r, fill_color.g, fill_color.b, fill_color.a);
    cairo_fill_preserve(context);

    if (fabs(rect.stroke.width) > 0.00001)
    {
      cairo_set_line_width(context, rect.stroke.width);
      Rgb stroke_color = rect.stroke.color;
      // TODO: Using rgba on PDF/Postfix/Print might degrade to bitmaps even if alpha is fully
      // opaque.
      //       Confirm if this is true or not.
      cairo_set_source_rgba(context, stroke_color.r, stroke_color.g, stroke_color.b,
                            stroke_color.a);

      cairo_stroke(context);
    }
    else
    {
      cairo_new_path(context);
    }

    cairo_restore(context);
  }

 private:
  cairo_t* context;
  Body::Handle body;
};

void CairoRenderSystem::update(entityx::EntityManager& es, entityx::EventManager& events,
                               entityx::TimeDelta dt)
{
  assert(context);
  cairo_save(context);

  Body::Handle body;
  Renderable::Handle renderable;

  m_orderedEntities.clear();
  m_orderedEntities.reserve(es.capacity());

  for (entityx::Entity entity : es.entities_with_components(body, renderable))
    m_orderedEntities.push_back(std::make_tuple(renderable, body));

  stable_sort(m_orderedEntities.begin(), m_orderedEntities.end(), [](EntityPack e1, EntityPack e2)
              {
                return get<0>(e1)->z < get<0>(e2)->z;
              });
  for (auto ep : m_orderedEntities)
    boost::apply_visitor(render_visitor(context, get<1>(ep)), get<0>(ep)->primitive);

  cairo_restore(context);
}
}