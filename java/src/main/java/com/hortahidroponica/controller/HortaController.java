package com.hortahidroponica.controller;

import com.hortahidroponica.model.Horta;
import com.hortahidroponica.model.Parametro;
import jakarta.transaction.Transactional;
import jakarta.ws.rs.*;
import jakarta.ws.rs.core.MediaType;
import jakarta.ws.rs.core.Response;


import java.util.List;

@Path("/horta")
public class HortaController {

    @GET
    @Produces(MediaType.APPLICATION_JSON)
    public List<Horta> getHorta() {
        return Horta.listAll();
    }

    @GET
    @Path("/id/{id}")
    @Produces(MediaType.APPLICATION_JSON)
    public Horta getHortaById(@PathParam("id") int id) {
        Horta horta = Horta.findById(id);

        if (horta == null) {
            throw new jakarta.ws.rs.NotFoundException("Horta com id " + id + " não encontrada.");
        }

        return horta;
    }

    @GET
    @Path("/nome/{nome}")
    @Produces(MediaType.APPLICATION_JSON)
    @Transactional
    public Response searchParameterByGardenName(@PathParam("nome") String nome) {
        Horta horta = Horta.find("nome", nome).firstResult();

        if (horta == null) {
            Horta newHorta = new Horta();
            newHorta.nome = nome;
            newHorta.status = 0;
            newHorta.persist();
            return Response.status(Response.Status.BAD_REQUEST)
                    .entity("Horta não encontrada, mas foi criada criada")
                    .build();
        }

        if (horta.parametro == null) {
            return Response.status(Response.Status.NOT_FOUND)
                    .entity("Horta encontrada, mas sem parâmetro associado")
                    .build();

        }

        return Response.ok(horta.parametro).build();
    }

}